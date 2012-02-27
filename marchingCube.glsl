#version 410 core

uniform mat4 uModelViewProjection;
uniform int uCase;

//uniform isamplerBuffer sEdgeConnectList;

layout(std140)  uniform CaseToNumPolys {
	ivec4 uCaseToNumPolys[64];
};
layout(std140)  uniform EdgeConnectList {
	ivec4 uEdgeConnectList[320];
};


#ifdef _VERTEX_

layout(location=0) in vec4 iDummy;

void main() {
	// empty
}
#endif //_VERTEX_


#ifdef _GEOMETRY_
/*
 *         3D  coordinate system Vertices
 *
 *             [+z] 
 *              |
 *              |
 *              |
 *              |
 *              |
 *              |
 *              +----------------[+y]
 *             /
 *            /
 *           /
 *          /
 *        [+x]
 *
 *
 *         Voxel Vertices
 *
 *             v5 //////////////// v6
 *            //|                // |
 *           // |               //  |
 *          //  |              //   |
 *         v1 /////////////// v2    |
 *         ||   |             ||    |
           ||   |             ||    |
 *         ||   v4------------||-- v7
 *         ||  /              ||   /
 *         || /               ||  /
 *         ||/                || /
 *         ||                 ||/
 *         v0 /////////////// v3
 *
 *
 *         Voxel Edges
 *
 *             //////////5//////////+
 *            //|                // |
 *           9/ |              10/  |
 *          //  4              //   |
 *         //////////1//////////    6
 *         ||   |             ||    |
           ||   |             ||    |
 *         ||   +--------7----||--- +
 *         0|  /              2|   /
 *         || 8               ||  11
 *         ||/                || /
 *         ||                 ||/
 *         +/////////3/////////
 *
 */


layout(points)         in;
layout(triangle_strip, max_vertices = 15)  out;

void main() {
	// compute the edges of the voxel
	float voxelHalfSize = 0.5;
	float xmin = -voxelHalfSize;
	float xmax = +voxelHalfSize;
	float ymin = -voxelHalfSize;
	float ymax = +voxelHalfSize;
	float zmin = -voxelHalfSize;
	float zmax = +voxelHalfSize;

	// follow gpu gems3 convention
	vec3 voxelVertices[8];
	voxelVertices[0] = vec3(xmax, ymin, zmin);
	voxelVertices[1] = vec3(xmax, ymin, zmax);
	voxelVertices[2] = vec3(xmax, ymax, zmax);
	voxelVertices[3] = vec3(xmax, ymax, zmin);
	voxelVertices[4] = vec3(xmin, ymin, zmin);
	voxelVertices[5] = vec3(xmin, ymin, zmax);
	voxelVertices[6] = vec3(xmin, ymax, zmax);
	voxelVertices[7] = vec3(xmin, ymax, zmin);

	// emit vertices using the marching cube tables
	int numPolys = uCaseToNumPolys[uCase/4][uCase%4];
	int i = 0;
	int edgeList, vindex0, vindex1;
	vec3 vertex;
	while(i<numPolys) {
//		edgeList   = texelFetch(sEdgeConnectList, uCase * 5 + i).r;
		int offset = uCase*5 + i;
		edgeList   = uEdgeConnectList[offset/4][offset%4];
		vindex0 = edgeList    & 0x7;
		vindex1 = edgeList>>3 & 0x7;
		vertex = 0.5 * voxelVertices[vindex1] + 0.5 * voxelVertices[vindex0];
		gl_Position = uModelViewProjection * vec4(vertex,1.0);
		EmitVertex();
		vindex0 = edgeList>>6 & 0x7;
		vindex1 = edgeList>>9 & 0x7;
		vertex = 0.5 * voxelVertices[vindex1] + 0.5 * voxelVertices[vindex0];
		gl_Position = uModelViewProjection * vec4(vertex,1.0);
		EmitVertex();
		vindex0 = edgeList>>12 & 0x7;
		vindex1 = edgeList>>15 & 0x7;
		vertex = 0.5 * voxelVertices[vindex1] + 0.5 * voxelVertices[vindex0];
		gl_Position = uModelViewProjection * vec4(vertex,1.0);
		EmitVertex();
		EndPrimitive();
		++i;
	}
}
#endif //_GEOMETRY_


#ifdef _FRAGMENT_
layout(location=0) out vec4 oColour;

void main() {
	oColour = vec4(1.0,0.0,0.0,0.0);
}
#endif //_FRAGMENT_
