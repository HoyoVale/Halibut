struct VertexInput
{
    [[vk::location(0)]] float3 pos;
    [[vk::location(1)]] float3 normal;
    [[vk::location(2)]] float2 uv;
};

struct VertexOutput
{
    [[vk::location(0)]] float3 worldPosition;
    [[vk::location(1)]] float3 worldNormal;
    [[vk::location(2)]] float2 uv;
    float4 sv_position : SV_Position;
};

struct ShadowVertexOutput
{
    [[vk::location(0)]] float3 worldPosition;
    float4 sv_position : SV_Position;
};
struct CameraBuffer
{
    float4x4 view;
    float4x4 projection;
    float4x4 lightView;
    float4x4 lightProjection;
    float4x4 cascadeLightViews[3];
    float4x4 cascadeLightProjections[3];
    float4 positionWorld;
    float4 lightDirectionWorld;
    float4 lightColorIntensity;
    float4 shadowParams;
    float4 cascadeSplits;
    float4 shadowFilterParams;
    float4 debugViewParams;
    float4 colorPipelineParams;
};

struct MaterialBuffer
{
    float4 baseTint;
    float4 emissiveTint;
    float4 surfaceParams;
};

struct ObjectPushConstants
{
    float4x4 model;
    float4 outlineColor;
    float4 drawFlags;
    uint4 objectInfo;
};

[[vk::binding(0, 0)]]
ConstantBuffer<CameraBuffer> camera;
[[vk::binding(0, 1)]]
Texture2D baseTexture;
[[vk::binding(1, 1)]]
SamplerState baseSampler;
[[vk::binding(2, 1)]]
Texture2D emissiveTexture;
[[vk::binding(3, 1)]]
SamplerState emissiveSampler;
[[vk::binding(4, 1)]]
Texture2D normalTexture;
[[vk::binding(5, 1)]]
SamplerState normalSampler;
[[vk::binding(6, 1)]]
ConstantBuffer<MaterialBuffer> material;
[[vk::binding(0, 2)]]
Texture2D shadowMapTexture0;
[[vk::binding(1, 2)]]
SamplerState shadowMapSampler0;
[[vk::binding(2, 2)]]
Texture2D shadowMapTexture1;
[[vk::binding(3, 2)]]
SamplerState shadowMapSampler1;
[[vk::binding(4, 2)]]
Texture2D shadowMapTexture2;
[[vk::binding(5, 2)]]
SamplerState shadowMapSampler2;
[[vk::push_constant]]
ConstantBuffer<ObjectPushConstants> objectData;

static const float PI = 3.14159265;

float4 EncodeObjectIdColor(uint objectId)
{
    uint r = objectId & 255u;
    uint g = (objectId >> 8) & 255u;
    uint b = (objectId >> 16) & 255u;
    return float4(float(r), float(g), float(b), 255.0) / 255.0;
}

float3 ApplyTonemap(float3 color, int tonemapMode)
{
    if (tonemapMode == 1)
    {
        return color / (color + 1.0.xxx);
    }
    if (tonemapMode == 2)
    {
        const float a = 2.51;
        const float b = 0.03;
        const float c = 2.43;
        const float d = 0.59;
        const float e = 0.14;
        return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
    }
    return color;
}

float3 ApplyColorPipeline(float3 colorLinear)
{
    float exposure = max(camera.colorPipelineParams.x, 0.001);
    int tonemapMode = (int)(camera.colorPipelineParams.y + 0.5);
    int gammaMode = (int)(camera.colorPipelineParams.z + 0.5); // 0 Auto, 1 Off, 2 On
    bool targetIsSrgb = camera.colorPipelineParams.w > 0.5;

    float3 color = colorLinear * exposure;
    color = ApplyTonemap(color, tonemapMode);

    bool applyGamma = false;
    if (gammaMode == 2)
    {
        applyGamma = true;
    }
    else if (gammaMode == 0 && !targetIsSrgb)
    {
        applyGamma = true;
    }

    if (applyGamma)
    {
        color = pow(saturate(color), 1.0 / 2.2);
    }
    return color;
}

float3x3 ComputeCotangentFrame(float3 normal, float3 worldPosition, float2 uv)
{
    float3 dp1 = ddx(worldPosition);
    float3 dp2 = ddy(worldPosition);
    float2 duv1 = ddx(uv);
    float2 duv2 = ddy(uv);

    float3 dp2Perp = cross(dp2, normal);
    float3 dp1Perp = cross(normal, dp1);
    float3 tangent = dp2Perp * duv1.x + dp1Perp * duv2.x;
    float3 bitangent = dp2Perp * duv1.y + dp1Perp * duv2.y;
    float invMax = rsqrt(max(dot(tangent, tangent), dot(bitangent, bitangent)));
    return float3x3(tangent * invMax, bitangent * invMax, normal);
}

float ComputeLinearDepth01(float depth, float nearPlane, float farPlane, bool invertDepth)
{
    float safeNear = max(nearPlane, 0.0001);
    float safeFar = max(farPlane, safeNear + 0.0001);
    float linearDepth = (safeNear * safeFar) / max(safeFar - depth * (safeFar - safeNear), 0.0001);
    float depth01 = saturate((linearDepth - safeNear) / (safeFar - safeNear));
    if (invertDepth)
    {
        depth01 = 1.0 - depth01;
    }
    return depth01;
}

int SelectShadowCascade(float viewDepth)
{
    const int cascadeCount = clamp((int)(camera.cascadeSplits.w + 0.5), 1, 3);
    if (cascadeCount <= 1)
    {
        return 0;
    }

    if (viewDepth <= camera.cascadeSplits.x)
    {
        return 0;
    }
    if (cascadeCount == 2 || viewDepth <= camera.cascadeSplits.y)
    {
        return 1;
    }
    return 2;
}

float SampleShadowDepth(int cascadeIndex, float2 uv)
{
    if (cascadeIndex == 0)
    {
        return shadowMapTexture0.Sample(shadowMapSampler0, uv).r;
    }
    if (cascadeIndex == 1)
    {
        return shadowMapTexture1.Sample(shadowMapSampler1, uv).r;
    }
    return shadowMapTexture2.Sample(shadowMapSampler2, uv).r;
}

float2 GetShadowTexelSize(int cascadeIndex)
{
    uint shadowMapWidth = 1;
    uint shadowMapHeight = 1;
    if (cascadeIndex == 0)
    {
        shadowMapTexture0.GetDimensions(shadowMapWidth, shadowMapHeight);
    }
    else if (cascadeIndex == 1)
    {
        shadowMapTexture1.GetDimensions(shadowMapWidth, shadowMapHeight);
    }
    else
    {
        shadowMapTexture2.GetDimensions(shadowMapWidth, shadowMapHeight);
    }

    float pcfTexelScale = max(camera.shadowFilterParams.y, 0.25);
    return pcfTexelScale / float2(max(shadowMapWidth, 1), max(shadowMapHeight, 1));
}

float SampleShadowCascade(int cascadeIndex, float3 worldPosition)
{
    float4x4 selectedLightView = camera.cascadeLightViews[cascadeIndex];
    float4x4 selectedLightProjection = camera.cascadeLightProjections[cascadeIndex];
    float4 lightViewPosition = mul(selectedLightView, float4(worldPosition, 1.0));
    float4 lightClipPosition = mul(selectedLightProjection, mul(selectedLightView, float4(worldPosition, 1.0)));
    if (lightClipPosition.w <= 0.0001)
    {
        return 1.0;
    }

    float3 lightNdcPosition = lightClipPosition.xyz / lightClipPosition.w;
    float2 shadowUv = lightNdcPosition.xy * 0.5 + 0.5;
    if (shadowUv.x < 0.0 || shadowUv.x > 1.0 || shadowUv.y < 0.0 || shadowUv.y > 1.0)
    {
        return 1.0;
    }

    float nearPlane = max(camera.shadowParams.x, 0.0001);
    float farPlane = max(camera.shadowParams.y, nearPlane + 0.0001);
    float shadowBias = max(camera.shadowParams.z, 0.0);
    float shadowStrength = saturate(camera.shadowParams.w);
    float receiverLinearDepth = -lightViewPosition.z;
    float receiverDepth01 = saturate((receiverLinearDepth - nearPlane) / max(farPlane - nearPlane, 0.0001));

    float2 texelSize = GetShadowTexelSize(cascadeIndex);
    int pcfRadius = clamp((int)(camera.shadowFilterParams.x + 0.5), 0, 4);
    float litSamples = 0.0;
    const float sampleCount = (pcfRadius * 2 + 1) * (pcfRadius * 2 + 1);
    for (int y = -pcfRadius; y <= pcfRadius; ++y)
    {
        for (int x = -pcfRadius; x <= pcfRadius; ++x)
        {
            float2 sampleUv = shadowUv + float2(x, y) * texelSize;
            float storedDepth01 = SampleShadowDepth(cascadeIndex, sampleUv);
            litSamples += (receiverDepth01 - shadowBias) <= storedDepth01 ? 1.0 : 0.0;
        }
    }

    float litFraction = litSamples / sampleCount;
    return 1.0 - shadowStrength * (1.0 - litFraction);
}

float SampleShadowFactor(float3 worldPosition, float viewDepth)
{
    int cascadeIndex = SelectShadowCascade(viewDepth);
    float shadowFactor = SampleShadowCascade(cascadeIndex, worldPosition);

    const int cascadeCount = clamp((int)(camera.cascadeSplits.w + 0.5), 1, 3);
    if (cascadeIndex >= cascadeCount - 1)
    {
        return shadowFactor;
    }

    float previousSplit = cascadeIndex == 0 ? max(camera.shadowParams.x, 0.0001) : camera.cascadeSplits[cascadeIndex - 1];
    float currentSplit = camera.cascadeSplits[cascadeIndex];
    float cascadeDepthRange = max(currentSplit - previousSplit, 0.0001);
    float transitionRange = max(cascadeDepthRange * clamp(camera.shadowFilterParams.z, 0.0, 0.45), 0.0001);
    float blendFactor = saturate((viewDepth - (currentSplit - transitionRange)) / transitionRange);
    if (blendFactor <= 0.0)
    {
        return shadowFactor;
    }

    float nextCascadeShadow = SampleShadowCascade(cascadeIndex + 1, worldPosition);
    return lerp(shadowFactor, nextCascadeShadow, blendFactor);
}

[shader("vertex")]
VertexOutput vertMain(VertexInput input)
{
    VertexOutput output;
    float4 worldPosition = mul(objectData.model, float4(input.pos, 1.0));
    float4 viewPosition = mul(camera.view, worldPosition);
    output.sv_position = mul(camera.projection, viewPosition);
    output.worldPosition = worldPosition.xyz;
    float3x3 normalMatrix = (float3x3)objectData.model;
    output.worldNormal = normalize(mul(normalMatrix, input.normal));
    output.uv = input.uv;
    return output;
}

[shader("vertex")]
ShadowVertexOutput vertShadowMain(VertexInput input)
{
    ShadowVertexOutput output;
    float4 worldPosition = mul(objectData.model, float4(input.pos, 1.0));
    float4 viewPosition = mul(camera.view, worldPosition);
    output.sv_position = mul(camera.projection, viewPosition);
    output.worldPosition = worldPosition.xyz;
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target
{
    if (objectData.objectInfo.x > 0u)
    {
        return EncodeObjectIdColor(objectData.objectInfo.y);
    }

    if (objectData.drawFlags.x > 0.5)
    {
        return float4(ApplyColorPipeline(objectData.outlineColor.rgb * objectData.outlineColor.a), 1.0);
    }

    int debugViewMode = (int)(camera.debugViewParams.x + 0.5);
    float4 baseTexel = baseTexture.Sample(baseSampler, inVert.uv);
    float3 baseNormal = normalize(inVert.worldNormal);
    float3 normal = baseNormal;
    if (material.surfaceParams.y > 0.0001)
    {
        float3 sampledNormal = normalTexture.Sample(normalSampler, inVert.uv).xyz * 2.0 - 1.0;
        sampledNormal.xy *= material.surfaceParams.y;
        float3x3 tangentFrame = ComputeCotangentFrame(baseNormal, inVert.worldPosition, inVert.uv);
        normal = normalize(mul(sampledNormal, tangentFrame));
    }
    if (debugViewMode == 1)
    {
        return float4(normal * 0.5 + 0.5, 1.0);
    }
    if (debugViewMode == 2)
    {
        float depth01 = ComputeLinearDepth01(
            saturate(inVert.sv_position.z),
            max(camera.debugViewParams.y, 0.0001),
            max(camera.debugViewParams.z, max(camera.debugViewParams.y, 0.0001) + 0.0001),
            camera.debugViewParams.w > 0.5
        );
        return float4(depth01.xxx, 1.0);
    }
    float3 albedo = baseTexel.rgb * material.baseTint.rgb;
    if (debugViewMode == 3)
    {
        return float4(ApplyColorPipeline(albedo), 1.0);
    }
    if (debugViewMode == 4)
    {
        float roughnessDebug = clamp(material.surfaceParams.w, 0.04, 1.0);
        return float4(roughnessDebug.xxx, 1.0);
    }
    if (debugViewMode == 5)
    {
        float metallicDebug = saturate(material.surfaceParams.z);
        return float4(metallicDebug.xxx, 1.0);
    }
    float3 emissiveColor = emissiveTexture.Sample(emissiveSampler, inVert.uv).rgb * material.emissiveTint.rgb * material.surfaceParams.x;
    if (debugViewMode == 6)
    {
        return float4(ApplyColorPipeline(emissiveColor), 1.0);
    }
    if (debugViewMode == 7)
    {
        float2 tiledUv = inVert.uv * 8.0;
        float2 cellId = floor(tiledUv);
        float checker = fmod(cellId.x + cellId.y, 2.0);
        float3 checkerColor = lerp(float3(0.12, 0.12, 0.12), float3(0.88, 0.88, 0.88), checker);
        float2 fracUv = frac(tiledUv);
        float edgeDistance = min(min(fracUv.x, 1.0 - fracUv.x), min(fracUv.y, 1.0 - fracUv.y));
        float gridLine = 1.0 - smoothstep(0.0, 0.02, edgeDistance);
        float3 uvTint = float3(frac(inVert.uv.x), frac(inVert.uv.y), 0.0);
        float3 uvCheckerColor = lerp(checkerColor, uvTint, 0.25);
        uvCheckerColor = lerp(uvCheckerColor, float3(1.0, 0.2, 0.2), gridLine * 0.7);
        return float4(ApplyColorPipeline(uvCheckerColor), 1.0);
    }
    if (debugViewMode == 8)
    {
        return float4(ApplyColorPipeline(emissiveColor), 1.0);
    }
    float3 lightToSurface = normalize(camera.lightDirectionWorld.xyz);
    float3 lightColor = camera.lightColorIntensity.rgb * camera.lightColorIntensity.w;
    float3 viewPosition = mul(camera.view, float4(inVert.worldPosition, 1.0)).xyz;
    float viewDepth = -viewPosition.z;
    float shadowFactor = SampleShadowFactor(inVert.worldPosition, viewDepth);
    float NdotL = saturate(dot(normal, lightToSurface));
    float3 viewDirection = normalize(camera.positionWorld.xyz - inVert.worldPosition);
    float3 halfVector = normalize(lightToSurface + viewDirection);

    float metallic = saturate(material.surfaceParams.z);
    float roughness = clamp(material.surfaceParams.w, 0.04, 1.0);
    float alpha = roughness * roughness;
    float alphaSquared = alpha * alpha;
    float NdotV = saturate(dot(normal, viewDirection));
    float NdotH = saturate(dot(normal, halfVector));
    float VdotH = saturate(dot(viewDirection, halfVector));

    float denominator = (NdotH * NdotH) * (alphaSquared - 1.0) + 1.0;
    float distribution = alphaSquared / max(PI * denominator * denominator, 0.0001);

    float k = ((roughness + 1.0) * (roughness + 1.0)) / 8.0;
    float geometryView = NdotV / max(NdotV * (1.0 - k) + k, 0.0001);
    float geometryLight = NdotL / max(NdotL * (1.0 - k) + k, 0.0001);
    float geometry = geometryView * geometryLight;

    float3 F0 = lerp(0.04.xxx, albedo, metallic);
    float3 fresnel = F0 + (1.0.xxx - F0) * pow(1.0 - VdotH, 5.0);

    float3 numerator = distribution * geometry * fresnel;
    float specularDenominator = max(4.0 * NdotV * NdotL, 0.0001);
    float3 specular = numerator / specularDenominator;
    float3 kS = fresnel;
    float3 kD = (1.0.xxx - kS) * (1.0 - metallic);
    float3 diffuse = kD * albedo / PI;

    float3 ambientLighting = 0.12.xxx * albedo;
    float3 directLighting = (diffuse + specular) * NdotL * lightColor * shadowFactor;
    float3 baseColor = ambientLighting + directLighting;
    float3 outputColor = ApplyColorPipeline(baseColor + emissiveColor);
    float baseAlpha = baseTexel.a * material.baseTint.a;
    return float4(outputColor, baseAlpha);
}

[shader("fragment")]
float4 fragMainDznFallback(VertexOutput inVert) : SV_Target
{
    if (objectData.objectInfo.x > 0u)
    {
        return EncodeObjectIdColor(objectData.objectInfo.y);
    }

    if (objectData.drawFlags.x > 0.5)
    {
        return float4(ApplyColorPipeline(objectData.outlineColor.rgb * objectData.outlineColor.a), 1.0);
    }

    int debugViewMode = (int)(camera.debugViewParams.x + 0.5);
    float3 normal = normalize(inVert.worldNormal);
    if (debugViewMode == 1)
    {
        return float4(normal * 0.5 + 0.5, 1.0);
    }
    if (debugViewMode == 2)
    {
        float depth01 = ComputeLinearDepth01(
            saturate(inVert.sv_position.z),
            max(camera.debugViewParams.y, 0.0001),
            max(camera.debugViewParams.z, max(camera.debugViewParams.y, 0.0001) + 0.0001),
            camera.debugViewParams.w > 0.5
        );
        return float4(depth01.xxx, 1.0);
    }

    float3 baseColor = float3(0.72, 0.74, 0.78);
    if (debugViewMode == 3)
    {
        return float4(ApplyColorPipeline(baseColor), 1.0);
    }

    float3 lightDirection = camera.lightDirectionWorld.xyz;
    if (dot(lightDirection, lightDirection) < 1e-6)
    {
        lightDirection = float3(0.4, 0.8, 0.4);
    }
    lightDirection = normalize(lightDirection);

    float3 lightColor = camera.lightColorIntensity.rgb * max(camera.lightColorIntensity.w, 0.0);
    float NdotL = saturate(dot(normal, lightDirection));
    float3 ambientLighting = 0.18.xxx * baseColor;
    float3 directLighting = baseColor * lightColor * NdotL;
    float3 outputColor = ApplyColorPipeline(ambientLighting + directLighting);
    return float4(outputColor, 1.0);
}

[shader("fragment")]
float4 fragShadowMap(ShadowVertexOutput inVert) : SV_Target
{
    float nearPlane = max(camera.debugViewParams.y, 0.0001);
    float farPlane = max(camera.debugViewParams.z, nearPlane + 0.0001);
    float4 lightViewPosition = mul(camera.view, float4(inVert.worldPosition, 1.0));
    float linearDepth = -lightViewPosition.z;
    float depth01 = saturate((linearDepth - nearPlane) / max(farPlane - nearPlane, 0.0001));
    if (camera.debugViewParams.w > 0.5)
    {
        depth01 = 1.0 - depth01;
    }
    return float4(depth01.xxx, 1.0);
}
