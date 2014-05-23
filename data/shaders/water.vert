// Shader based on work by Fabien Sanglard
// Released under the terms of CC-BY 3.0
//#version 330 compatibility

uniform mat4 ModelViewMatrix;
uniform mat4 ModelViewProjectionMatrix;
uniform mat3 NormalMatrix;

uniform float time;

// Future uniform variables
const float speed = 2.0;
const float height = 0.75;
const float waveLength = 2.0;

const vec3 lightdir = vec3(1.0, 1.0, 0.0);


#if __VERSION__ >= 130
in vec3 Position;
in vec2 Texcoord;
in vec3 Normal;
out vec3 lightVec;
out vec3 halfVec;
out vec3 eyeVec;
out vec2 uv;
#else
attribute vec3 Position;
attribute vec2 Texcoord;
attribute vec3 Normal;
varying vec3 lightVec;
varying vec3 halfVec;
varying vec3 eyeVec;
varying vec2 uv;
#endif

void main()
{
	vec4 pos = /*gl_Vertex*/vec4(Position, 1.0);

	pos.y += (sin(pos.x/waveLength + speed*time) + cos(pos.z/waveLength + speed*time)) * height;

	vec3 vertexPosition = vec3(/*gl_ModelViewMatrix*/ModelViewMatrix * pos);

	// Building the matrix Eye Space -> Tangent Space
	vec3 n = normalize (/*gl_NormalMatrix*/NormalMatrix * /*gl_Normal*/Normal);
	// gl_MultiTexCoord1.xyz
	vec3 t = normalize (/*gl_NormalMatrix*/NormalMatrix * vec3(1.0, 0.0, 0.0)); // tangent
	vec3 b = cross (n, t);

	// transform light and half angle vectors by tangent basis
	vec3 v;
	v.x = dot (lightdir, t);
	v.y = dot (lightdir, b);
	v.z = dot (lightdir, n);
	lightVec = normalize (v);

	vertexPosition = normalize(vertexPosition);

	eyeVec = normalize(-vertexPosition); // we are in Eye Coordinates, so EyePos is (0,0,0)

	// Normalize the halfVector to pass it to the fragment shader

	// No need to divide by two, the result is normalized anyway.
	// vec3 halfVector = normalize((vertexPosition + lightDir) / 2.0);
	vec3 halfVector = normalize(vertexPosition + lightdir);
	v.x = dot (halfVector, t);
	v.y = dot (halfVector, b);
	v.z = dot (halfVector, n);

	// No need to normalize, t,b,n and halfVector are normal vectors.
	//normalize (v);
	halfVec = v ;

	gl_Position = /*gl_ModelViewProjectionMatrix*/ModelViewProjectionMatrix * pos;
	//uv = (gl_TextureMatrix[0] * gl_MultiTexCoord0).st;
	uv = Texcoord;
}