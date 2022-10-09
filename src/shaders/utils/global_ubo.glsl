struct GlobalUbo {
    mat4 view;
    mat4 proj;
    mat4 projView;
    vec4 ambientLightColor; // w is intensity
    vec4 lightPosition; // w is ignored
    vec4 lightColor; // w is intensity
};