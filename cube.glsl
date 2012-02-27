#version 410

uniform mat4 uModelViewProjection;


#ifdef _VERTEX_
layout(location = 0)  in vec4 iPosition;

void main() {
	gl_Position = uModelViewProjection * iPosition;
}
#endif


#ifdef _FRAGMENT_
layout(location = 0)  out vec4 oColour;

void main() {
	oColour = vec4(1.0);
}
#endif // _FRAGMENT_
