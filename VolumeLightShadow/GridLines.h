#pragma once
#include "RObject.h"

class CGrid :public RObject
{
public:
	CGrid(int width = 10, int depth = 10);
	virtual ~CGrid(void);

	int GetTotalVertices();
	int GetTotalIndices();
	GLenum GetPrimitiveType();

	void FillVertexBuffer(GLfloat* pBuffer);
	void FillIndexBuffer(GLuint* pBuffer);

private:
	int width, depth;
};

