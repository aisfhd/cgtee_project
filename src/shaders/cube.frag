#version 330 core
in vec3 FragmentColor;
out vec4 FinalColor;

void main()
{
    FinalColor = vec4(FragmentColor, 1.0);
}