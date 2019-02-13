#version 330 core

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

uniform DirLight dirLight;
uniform SpotLight spotLight;

uniform int pointLightNumber;
#define MAX_POINT_LIGHT_NUMBER 4
uniform PointLight pointLights[MAX_POINT_LIGHT_NUMBER];

uniform Material material;
uniform vec3 viewPos;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

out vec4 FragColor;


vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLightSoft(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{

    // properties
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // phase 1: Directional lighting
    vec3 result = CalcDirLight(dirLight, norm, viewDir);

    // phase 2: Point lights
    for(int i = 0; i < pointLightNumber; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);

    //phase 3: Spot light
    result += CalcSpotLightSoft(spotLight, norm, FragPos, viewDir);

    FragColor = vec4(result, 1.0);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // combine results
    vec3 ambient  = light.ambient  * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    return (ambient + diffuse + specular);
}


vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // attenuation
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance +
  			     light.quadratic * (distance * distance));
    // combine results
    vec3 ambient  = light.ambient  * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

vec3 CalcSpotLightSoft(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    // ambient
    vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;

    // diffuse
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;

    // specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * texture(material.specular, TexCoords).rgb;

    // spotlight (soft edges)
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = (light.cutOff - light.outerCutOff);
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    diffuse  *= intensity;
    specular *= intensity;

    // attenuation
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    ambient  *= attenuation;
    diffuse   *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
     vec3 lightDir = normalize(light.position - fragPos);

     // check if lighting is inside the spotlight cone
     float theta = dot(lightDir, normalize(-light.direction));

     // remember that we're working with angles as cosines instead of degrees so a '>' is used.
     if(theta > light.cutOff){
         // ambient
         vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;

         // diffuse
         vec3 norm = normalize(Normal);
         float diff = max(dot(norm, lightDir), 0.0);
         vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;

         // specular
         vec3 reflectDir = reflect(-lightDir, norm);
         float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
         vec3 specular = light.specular * spec * texture(material.specular, TexCoords).rgb;

         // attenuation
         float distance    = length(light.position - fragPos);
         float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

         diffuse   *= attenuation;
         specular *= attenuation;

         return ambient + diffuse + specular;
     }else{
         // else, use ambient light so scene isn't completely dark outside the spotlight.
         return light.ambient * texture(material.diffuse, TexCoords).rgb;
     }
