#include "RenderingManager.h"

bool RenderingManager::init()
{
	skyboxShaderProgram = new ShaderProgram({
		ShaderDir "Skybox.vert",
		ShaderDir "Skybox.frag" });

	depthShaderProgram = new ShaderProgram({
		ShaderDir "DepthShader.vert",
		ShaderDir "DepthShader.frag" });

	blobShaderProgram = new ShaderProgram({
		ShaderDir "Blob.vert",
		ShaderDir "Blob.tesc",
		ShaderDir "Blob.tese",
		ShaderDir "Blob.frag" });

	platformShaderProgram = new ShaderProgram({
		ShaderDir "Platform.vert",
		ShaderDir "Platform.frag" });

	quadShaderProgram = new ShaderProgram({
		ShaderDir "toQuad.vert",
		ShaderDir "toQuad.frag" });

	geomPassShaderProgram = new ShaderProgram({
		ShaderDir "GeometryPass.vert",
		ShaderDir "GeometryPass.frag" });

	SSAOShaderProgram = new ShaderProgram({
		ShaderDir "SSAO.vert",
		ShaderDir "SSAO.frag" });

	blurShaderProgram = new ShaderProgram({
		ShaderDir "SSAO.vert",
		ShaderDir "Blur.frag" });

	particleShaderProgram = new ShaderProgram({
		ShaderDir "Particle.vert",
		ShaderDir "Particle.frag" });

	dirLight.color = glm::vec3(1.0f, 1.0f, 1.0f);
	dirLight.direction = glm::vec3(-0.2f, -0.4f, -0.2f);

	width = RENDER_WIDTH;
	height = RENDER_HEIGHT;

	quad = Mesh::CreateQuad();

	initSkybox();

	// For shadows
	glm::mat4 lightProjection = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, 0.1f, 200.0f);
	glm::mat4 lightView = glm::lookAt(-dirLight.direction, glm::vec3(0.0f), glm::vec3(1.0f));
	lightSpaceMatrix = lightProjection * lightView;

	// For ambient occlusion
	std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
	std::default_random_engine generator;
	for (GLuint i = 0; i < 64; ++i)
	{
		glm::vec3 sample(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator)
			);

		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		GLfloat scale = GLfloat(i) / 64.0;

		// Scale samples s.t. they're more aligned to center of kernel
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		ssaoKernel.push_back(sample);
	}

	// Noise texture
	std::vector<glm::vec3> ssaoNoise;
	for (GLuint i = 0; i < 16; i++)
	{
		// rotate around z-axis (in tangent space)
		glm::vec3 noise(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			0.0f);
		ssaoNoise.push_back(noise);
	}
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	if (!initFrameBuffers())
		return false;

	return true;
}

bool RenderingManager::initFrameBuffers()
{
	if (!gBuffer.Init(width, height, true, GL_RGB32F))
		return false;

	if (!aoBuffer.Init(width, height, false, GL_RED))
		return false;

	if (!blurBuffer.Init(width, height, false, GL_RED))
		return false;

	if (!cubeMapBuffer.Init(TEX_WIDTH, TEX_HEIGHT, true, GL_RGB))
		return false;

	if (!depthBuffer.Init(SHADOW_WIDTH, SHADOW_HEIGHT, false, GL_DEPTH_COMPONENT))
		return false;

	return true;
}

bool RenderingManager::initSkybox()
{
	skybox.buildCubeMesh();
	skybox.modelMat = glm::rotate(skybox.modelMat, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	std::vector<const GLchar*> faces;
	faces.push_back(TextureDir "sunny_skybox/negx.png");
	faces.push_back(TextureDir "sunny_skybox/posx.png");
	faces.push_back(TextureDir "sunny_skybox/posy.png");
	faces.push_back(TextureDir "sunny_skybox/negy.png");
	faces.push_back(TextureDir "sunny_skybox/posz.png");
	faces.push_back(TextureDir "sunny_skybox/negz.png");
	
	if (!skybox.loadCubeMap(faces))
		return false;

	return true;
}

void RenderingManager::drawBlob(Blob *blob, glm::vec3 camPos, glm::mat4 viewMatrix, glm::mat4 projMatrix)
{
	(*blobShaderProgram)["objectColor"] = glm::vec3(0.0f, 1.0f, 0.0f);
	(*blobShaderProgram)["directionalLight.color"] = dirLight.color;
	(*blobShaderProgram)["directionalLight.ambientColor"] = dirLight.ambientColor;
	(*blobShaderProgram)["directionalLight.direction"] = dirLight.direction;
	(*blobShaderProgram)["viewPos"] = camPos;
	(*blobShaderProgram)["blobDistance"] =
		glm::distance(convert(blob->GetCentroid()), camPos);

	(*blobShaderProgram)["projection"] = projMatrix;
	(*blobShaderProgram)["view"] = viewMatrix;
	(*blobShaderProgram)["lightSpaceMat"] = lightSpaceMatrix;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapBuffer.texture0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthBuffer.texture0);

	blobShaderProgram->Use([&]() {
		blob->RenderPatches();
	});
}

void RenderingManager::drawLevel(Level *level, glm::vec3 camPos, glm::mat4 viewMatrix, glm::mat4 projMatrix)
{
	(*platformShaderProgram)["directionalLight.color"] = dirLight.color;
	(*platformShaderProgram)["directionalLight.ambientColor"] = dirLight.ambientColor;
	(*platformShaderProgram)["directionalLight.direction"] = dirLight.direction;
	(*platformShaderProgram)["viewPos"] = camPos;
	(*platformShaderProgram)["screenSize"] = glm::vec2(RENDER_WIDTH, RENDER_HEIGHT);

	(*platformShaderProgram)["projection"] = projMatrix;
	(*platformShaderProgram)["view"] = viewMatrix;
	(*platformShaderProgram)["lightSpaceMat"] = lightSpaceMatrix;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthBuffer.texture0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, blurBuffer.texture0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	GLuint uMMatrix = platformShaderProgram->GetUniformLocation("model");
	GLuint uColor = platformShaderProgram->GetUniformLocation("objectColor");
	platformShaderProgram->Use([&]() {
		level->Render(uMMatrix, uColor);
	});
	glDisable(GL_CULL_FACE);
}

void RenderingManager::drawParticles(Level *level, 
	glm::mat4 viewMatrix, glm::mat4 projMatrix)
{
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glEnable(GL_BLEND);
	(*particleShaderProgram)["uProj"] = projMatrix;
	(*particleShaderProgram)["uView"] = viewMatrix;
	GLuint uSize = particleShaderProgram->GetUniformLocation("uSize");
	particleShaderProgram->Use([&]() {
		level->RenderParticles(uSize);
	});
	glDisable(GL_BLEND);
}

void RenderingManager::depthPass(Blob *blob, Level *level)
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthBuffer.FBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

	(*depthShaderProgram)["lightSpaceMat"] = lightSpaceMatrix;
	(*depthShaderProgram)["model"] = glm::mat4();
	depthShaderProgram->Use([&]() {
		blob->Render();
		GLuint uMMatrix = depthShaderProgram->GetUniformLocation("model");
		level->Render(uMMatrix, -1);
	});

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_CULL_FACE);
}

void RenderingManager::geometryPass(Level *level, glm::mat4 viewMatrix, glm::mat4 projMatrix)
{
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.FBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	(*geomPassShaderProgram)["projection"] = projMatrix;
	(*geomPassShaderProgram)["view"] = viewMatrix;
	GLuint uMMatrix = geomPassShaderProgram->GetUniformLocation("model");
	GLuint uColor = geomPassShaderProgram->GetUniformLocation("objectColor");
	geomPassShaderProgram->Use([&]() {
		level->Render(uMMatrix, uColor);
	});

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderingManager::SSAOPass(glm::mat4 projMatrix)
{
	glBindFramebuffer(GL_FRAMEBUFFER, aoBuffer.FBO);
	glClear(GL_COLOR_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gBuffer.texture0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gBuffer.texture1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);

	(*SSAOShaderProgram)["screenSize"] = glm::vec2(width, height);
	(*SSAOShaderProgram)["projection"] = projMatrix;
	SSAOShaderProgram->Use([&]() {
		for (GLuint i = 0; i < 64; ++i)
			glUniform3fv(glGetUniformLocation(SSAOShaderProgram->program, ("samples[" + std::to_string(i) + "]").c_str()), 1, &ssaoKernel[i][0]);
	});

	SSAOShaderProgram->Use([&]() {
		quad->Draw();
	});

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderingManager::blurPass()
{
	glBindFramebuffer(GL_FRAMEBUFFER, blurBuffer.FBO);
	glClear(GL_COLOR_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, aoBuffer.texture0);

	blurShaderProgram->Use([&]() {
		quad->Draw();
	});

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderingManager::dynamicCubeMapPass(Blob *blob, Level *level)
{
	glBindFramebuffer(GL_FRAMEBUFFER, cubeMapBuffer.FBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, TEX_WIDTH, TEX_HEIGHT);
	glm::mat4 projMatrix = glm::perspective(glm::radians(90.0f), (float)TEX_WIDTH / (float)TEX_HEIGHT, 0.1f, 500.0f);

	glm::vec3 position = convert(blob->GetCentroid());
	drawCubeFace(
		position,
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, -1.0f, 0.0f),
		GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		projMatrix,
		level);

	glClear(GL_DEPTH_BUFFER_BIT);

	drawCubeFace(position,
		glm::vec3(-1.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, -1.0f, 0.0f),
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		projMatrix,
		level);

	glClear(GL_DEPTH_BUFFER_BIT);

	drawCubeFace(position,
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(0.0f, -1.0f, 0.0f),
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		projMatrix,
		level);

	glClear(GL_DEPTH_BUFFER_BIT);

	drawCubeFace(position,
		glm::vec3(0.0f, 0.0f, -1.0f),
		glm::vec3(0.0f, -1.0f, 0.0f),
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
		projMatrix,
		level);

	glClear(GL_DEPTH_BUFFER_BIT);

	drawCubeFace(position,
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f),
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		projMatrix,
		level);

	glClear(GL_DEPTH_BUFFER_BIT);

	drawCubeFace(position,
		glm::vec3(0.0f, -1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, -1.0f),
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		projMatrix,
		level);

	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapBuffer.texture0);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderingManager::drawCubeFace(glm::vec3 position, glm::vec3 direction, glm::vec3 up,
	GLenum face, glm::mat4 projMatrix, Level *level)
{
	glm::mat4 viewMatrix = glm::lookAt(position, position + direction, up);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, face, cubeMapBuffer.texture0, 0);

	drawLevel(level, position, viewMatrix, projMatrix);
	viewMatrix = glm::mat4(glm::mat3(viewMatrix));
	drawSkybox(viewMatrix, projMatrix);
}

void RenderingManager::drawSkybox(glm::mat4 viewMatrix, glm::mat4 projMatrix)
{
	glDepthFunc(GL_LEQUAL);
	(*skyboxShaderProgram)["model"] = skybox.modelMat;
	(*skyboxShaderProgram)["view"] = viewMatrix;
	(*skyboxShaderProgram)["projection"] = projMatrix;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.getID());
	skyboxShaderProgram->Use([&]() {
		skybox.render();
	});

	glDepthFunc(GL_LESS);
}

void RenderingManager::debugQuadDraw()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gBuffer.texture1);
	quadShaderProgram->Use([&]() {
		quad->Draw();
	});
}