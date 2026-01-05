struct Cube
{
	glm::vec3 position;
	glm::vec3 scale;

	GLfloat normal_buffer_data[60] = {0};

	GLfloat color_buffer_data[60] = {
		// Front, white
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,

		// Back, white
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,

		// Left, white
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,

		// Right, white
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,

		// Top, white
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f,
		1.0f};

	GLuint index_buffer_data[30] = {
		// 12 triangle faces of a box
		0,
		1,
		2,
		0,
		2,
		3,

		4,
		5,
		6,
		4,
		6,
		7,

		8,
		9,
		10,
		8,
		10,
		11,

		12,
		13,
		14,
		12,
		14,
		15,

		16,
		17,
		18,
		16,
		18,
		19};

	// OpenGL buffers
	GLuint vertexArrayID;
	GLuint vertexBufferID;
	GLuint indexBufferID;
	GLuint colorBufferID;
	GLuint normalBufferID;

	GLuint mvpMatrixID;
	GLuint lightPositionID;
	GLuint lightIntensityID;
	GLuint lightSpaceMatrixID;
	GLuint shadowMapID;
	GLuint programID;


	void initialize(std::vector<GLfloat> &BoxData)
	{
		glGenVertexArrays(1, &vertexArrayID);
		glBindVertexArray(vertexArrayID);

		glGenBuffers(1, &vertexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * BoxData.size(), &BoxData[0], GL_STATIC_DRAW);

		glGenBuffers(1, &colorBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(color_buffer_data), color_buffer_data, GL_STATIC_DRAW);

		for (int i = 0; i < 5; i++)
		{
			glm::vec3 vector1(BoxData[i * 12 + 3] - BoxData[i * 12 + 0],
							  BoxData[i * 12 + 4] - BoxData[i * 12 + 1],
							  BoxData[i * 12 + 5] - BoxData[i * 12 + 2]);
			glm::vec3 vector2(BoxData[i * 12 + 6] - BoxData[i * 12 + 0],
							  BoxData[i * 12 + 7] - BoxData[i * 12 + 1],
							  BoxData[i * 12 + 8] - BoxData[i * 12 + 2]);
			glm::vec3 crossP = cross(vector1, vector2);
			glm::vec3 normalizedNormal = glm::normalize(crossP);
			for (int j = 0; j < 4; j++)
			{
				normal_buffer_data[12 * i + j * 3] = normalizedNormal.x;
				normal_buffer_data[12 * i + j * 3 + 1] = normalizedNormal.y;
				normal_buffer_data[12 * i + j * 3 + 2] = normalizedNormal.z;
			}
		}

		glGenBuffers(1, &normalBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(normal_buffer_data), normal_buffer_data, GL_STATIC_DRAW);

		glGenBuffers(1, &indexBufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer_data), index_buffer_data, GL_STATIC_DRAW);

		programID = LoadShadersFromFile("../lab3/box.vert", "../lab3/box.frag");
		if (programID == 0)
		{
			std::cerr << "Failed to load shaders." << std::endl;
		}

		mvpMatrixID = glGetUniformLocation(programID, "MVP");
		lightPositionID = glGetUniformLocation(programID, "lightPosition");
		lightIntensityID = glGetUniformLocation(programID, "lightIntensity");
		lightSpaceMatrixID = glGetUniformLocation(programID, "lightSpaceMatrix");
		shadowMapID = glGetUniformLocation(programID, "shadowMap");
	}
	void renderDepth(const glm::mat4 &depthMVP)
	{
		glUseProgram(depthProg);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

		glUniformMatrix4fv(depthMVP_ID, 1, GL_FALSE, &depthMVP[0][0]);

		glDrawElements(GL_TRIANGLES, 30, GL_UNSIGNED_INT, (void *)0);

		glDisableVertexAttribArray(0);
	}
	void render(glm::mat4 cameraMatrix)
	{
		glUseProgram(programID);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

		glm::mat4 mvp = cameraMatrix;
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

		glUniform3fv(lightPositionID, 1, &lightPosition[0]);
		glUniform3fv(lightIntensityID, 1, &lightIntensity[0]);
		glUniformMatrix4fv(lightSpaceMatrixID, 1, GL_FALSE, &gLightSpace[0][0]);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, shadowDepthTex);
		glUniform1i(shadowMapID, 0);

		glDrawElements(
			GL_TRIANGLES,
			30,
			GL_UNSIGNED_INT,
			(void *)0
		);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
	}

	void cleanup()
	{
		glDeleteBuffers(1, &vertexBufferID);
		glDeleteBuffers(1, &colorBufferID);
		glDeleteBuffers(1, &indexBufferID);
		glDeleteBuffers(1, &normalBufferID);
		glDeleteVertexArrays(1, &vertexArrayID);
		glDeleteProgram(programID);
	}
};
