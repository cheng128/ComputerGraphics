#version 330 core

out vec4 FragColor;
in vec3 vertex_color;
in vec3 vertex_normal;
in vec3 FragPos;

struct Material
{
    vec3 Ka;
    vec3 Kd;
    vec3 Ks;
    float shininess;
};

struct DirectionalLight
{
    vec3 Pos;
    vec3 Dir;
};

struct PointLight
{
    vec3 Pos;
};

struct attPoint
{
    float constant;
    float linear;
    float quadratic;
};

struct SpotLight
{
    vec3 Pos;
    vec3 Dir;
    float Expo;
    float Cut;
};

struct attSpot
{
    float constant;
    float linear;
    float quadratic;
};

struct enablePhong
{
    int diffuse;
    int specular;
};

uniform vec3 viewPos;
uniform vec3 diffuseIntensity;
uniform Material material;
uniform DirectionalLight DL;
uniform PointLight PL;
uniform SpotLight SL;

uniform enablePhong enPhong;
uniform int enSpotLightEff;

uniform int LightMode;
uniform int VertexPixel;

attPoint attPL = attPoint(0.01, 0.8, 0.1);
attSpot attSL = attSpot(0.05, 0.3, 0.6);

vec3 ambientIntensity = vec3(0.15, 0.15, 0.15);

vec3 Compute_Dir(){
    // ambient
    vec3 ambient = ambientIntensity * material.Ka;
    
    // diffuse
    vec3 lightDir = normalize(DL.Pos);
    vec3 diffuse = vec3(0, 0, 0);
    if (enPhong.diffuse == 1)
    {
        float diff = max(dot(normalize(vertex_normal), lightDir), 0.0);
        diffuse = diff * diffuseIntensity * material.Kd;
    }
    
    //specular
    vec3 specular = vec3(0, 0, 0);
    if (enPhong.specular == 1){
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 H = normalize(lightDir + viewDir);
        float spec = pow(max(dot(H, normalize(vertex_normal)), 0.0), material.shininess);
        specular = spec * material.Ks;
    }
    
    vec3 DL_result = ambient + diffuse + specular;
    
    return DL_result;
}

vec3 Compute_Point() {
    // attenuation
    float d = length(FragPos - PL.Pos);
    float attenuation = min(1.0 / (attPL.constant + attPL.linear * d + attPL.quadratic * d * d), 1.0);
    
    vec3 ambient = ambientIntensity * material.Ka;
    // diffuse
    vec3 diffuse = vec3(0, 0, 0);
    vec3 lightDir = normalize(PL.Pos - FragPos);
    if (enPhong.diffuse == 1){
        float diff = max(dot(normalize(vertex_normal), lightDir), 0.0);
        diffuse = diff * diffuseIntensity * material.Kd;
    }
    //specular
    vec3 specular = vec3(0, 0, 0);
    if (enPhong.specular == 1){
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 H = normalize(lightDir + viewDir);
        float spec = pow(max(dot(H, normalize(vertex_normal)), 0.0), material.shininess);
        specular = spec * material.Ks;
    }
    
    vec3 PL_result = ambient + attenuation * (diffuse + specular);
    return PL_result;
}

vec3 Compute_Spot() {
    vec3 SL_result;
    
    vec3 ambient = ambientIntensity * material.Ka;
    vec3 lightDir = normalize(SL.Pos - FragPos);
    // in or out spotlight
    float theta = dot(lightDir, normalize(-SL.Dir));
    if (theta > cos(SL.Cut)) {
        
        // attenuation
        float d = length(FragPos - SL.Pos);
        float attenuation = min(1.0 / (attSL.constant + attSL.linear * d + attSL.quadratic * d * d), 1.0);

        // diffuse
        vec3 diffuse = vec3(0, 0, 0);
        if (enPhong.diffuse == 1){
            float diff = max(dot(normalize(vertex_normal), lightDir), 0.0);
            diffuse = diff * diffuseIntensity * material.Kd;
        }
        
        //specular
        vec3 specular = vec3(0, 0, 0);
        if (enPhong.specular == 1){
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 H = normalize(lightDir + viewDir);
            float spec = pow(max(dot(H, normalize(vertex_normal)), 0.0), material.shininess);
            specular = spec * material.Ks;
        }

        float spotlightEff = pow(max(dot(normalize(FragPos - SL.Pos), normalize(SL.Dir)), 0.0), SL.Expo);
        
        if (enSpotLightEff == 1){
            SL_result = ambient + attenuation * spotlightEff * (diffuse + specular);
        } else {
            SL_result = ambient + attenuation * (diffuse + specular);
        }
                
    } else {
        SL_result = ambient;
    }
    return SL_result;
}

void main() {
	// [TODO]
    //per pixel
    if (VertexPixel == 1) {
        vec3 result;
        if (LightMode == 0){
            result = Compute_Dir();
        } else if (LightMode == 1) {
            result = Compute_Point();
        } else if (LightMode == 2) {
            result = Compute_Spot();
        }
        FragColor = vec4(result, 1.0);
    } else {
        // vertex lighting
        FragColor = vec4(vertex_color, 1.0);
    }
}
