#include "RenderingManager.h"
#define KERNEL_SIZE 32.0f

bool RenderingManager::init()
{
	skyboxShader = new ShaderProgram({
		ShaderDir "Skybox.vert",
		ShaderDir "Skybox.frag" });

	depthShader = new ShaderProgram({
		ShaderDir "DepthShader.vert",
		ShaderDir "DepthShader.frag" });

	blobShader = new ShaderProgram({
		ShaderDir "Blob.vert",
		ShaderDir "Blob.tesc",
		ShaderDir "Blob.tese",
		ShaderDir "Blob.frag" });

	platformShader = new ShaderProgram({
		ShaderDir "Platform.vert",
		ShaderDir "Platform.frag" });

	quadShader = new ShaderProgram({
		ShaderDir "toQuad.vert",
		ShaderDir "toQuad.frag" });

	geomPassShader = new ShaderProgram({
		ShaderDir "GeometryPass.vert",
		ShaderDir "GeometryPass.frag" });

	SSAOShader = new ShaderProgram({
		ShaderDir "SSAO.vert",
		ShaderDir "SSAO.frag" });

	blurShader = new ShaderProgram({
		ShaderDir "Blur.vert",
		ShaderDir "Blur.frag" });

	particleShader = new ShaderProgram({
		ShaderDir "Particle.vert",
		ShaderDir "Particle.frag" });

	dirLight.color = glm::vec3(1.0f, 1.0f, 1.0f);
	dirLight.direction = glm::vec3(-0.2f, -0.3f, -0.2f);

	width = RENDER_WIDTH;
	height = RENDER_HEIGHT;

	quad = Mesh::CreateQuad();

	initSkybox();

	// For blurring
	pingpongBuffers = std::vector<IOBuffer>(2);

	// Noise texture
	std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0);
	std::default_random_engine generator;
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

	wallTexture = loadTexture(TextureDir "metal.png", true);

	if (!initFrameBuffers())
		return false;

	return true;
}

bool RenderingManager::initFrameBuffers()
{
	if (!gBuffer.Init(width, height, true, GL_RGBA32F))
		return false;

	if (!aoBuffer.Init(AO_DIM, AO_DIM, false, GL_RED))
		return false;

	if (!pingpongBuffers[0].Init(AO_DIM, AO_DIM, false, GL_RGB16F))
		return false;

	if (!pingpongBuffers[1].Init(AO_DIM, AO_DIM, false, GL_RGB16F))
		return false;

	if (!cubeMapBuffer.Init(TEX_WIDTH, TEX_HEIGHT, true, GL_RGB))
		return false;

	if (!depthBuffer.Init(SHADOW_DIM, SHADOW_DIM, false, GL_DEPTH_COMPONENT))
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
	(*blobShader)["objectColor"] = glm::vec3(0.0f, 1.0f, 0.0f);
	(*blobShader)["directionalLight.color"] = dirLight.color;
	(*blobShader)["directionalLight.ambientColor"] = dirLight.ambientColor;
	(*blobShader)["directionalLight.direction"] = dirLight.direction;
	(*blobShader)["viewPos"] = camPos;
	(*blobShader)["blobDistance"] =
		glm::distance(convert(blob->GetCentroid()), camPos);

	(*blobShader)["projection"] = projMatrix;
	(*blobShader)["view"] = viewMatrix;
	(*blobShader)["lightSpaceMat"] = lightSpaceMatrix;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapBuffer.texture0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthBuffer.texture0);

	blobShader->Use([&]() {
		blob->RenderPatches();
	});
}

void RenderingManager::drawLevel(Level *level, glm::vec3 camPos, glm::mat4 viewMatrix, glm::mat4 projMatrix)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	(*platformShader)["directionalLight.color"] = dirLight.color;
	(*platformShader)["directionalLight.ambientColor"] = dirLight.ambientColor;
	(*platformShader)["directionalLight.direction"] = dirLight.direction;
	(*platformShader)["viewPos"] = camPos;
	(*platformShader)["screenSize"] = glm::vec2(width, height);

	(*platformShader)["projection"] = projMatrix;
	(*platformShader)["view"] = viewMatrix;
	(*platformShader)["lightSpaceMat"] = lightSpaceMatrix;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthBuffer.texture0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, pingpongBuffers[0].texture0);

	//glActiveTexture(GL_TEXTURE2);
	//glBindTexture(GL_TEXTURE_2D, wallTexture);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	GLuint uMMatrix = platformShader->GetUniformLocation("model");
	GLuint uColor = platformShader->GetUniformLocation("objectColor");
	platformShader->Use([&]() {
		level->Render(uMMatrix, uColor);
	});
	glDisable(GL_CULL_FACE);

	glDisable(GL_BLEND);
}

void RenderingManager::drawParticles(Level *level, 
	glm::mat4 viewMatrix, glm::mat4 projMatrix)
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glEnable(GL_BLEND);
	(*particleShader)["uProj"] = projMatrix;
	(*particleShader)["uView"] = viewMatrix;
	GLuint uSize = particleShader->GetUniformLocation("uSize");
	particleShader->Use([&]() {
		level->RenderParticles(uSize);
	});
	glDisable(GL_BLEND);
}

void RenderingManager::depthPass(Blob *blob, Level *level, glm::vec3 camPos)
{
	glViewport(0, 0, SHADOW_DIM, SHADOW_DIM);
	glBindFramebuffer(GL_FRAMEBUFFER, depthBuffer.FBO);
	glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthBuffer.texture0, 0, 0);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glClear(GL_DEPTH_BUFFER_BIT);

	glm::mat4 lightProjection = glm::ortho(100.0f + camPos.z/2, -200.0f + camPos.z/2, 100.0f + camPos.z/2, -200.0f + camPos.z/2, -100.0f, 200.0f);
	glm::mat4 lightView = glm::lookAt(glm::vec3(0.0f), dirLight.direction, glm::vec3(1.0f));
	lightSpaceMatrix = lightProjection * lightView;
	
	(*depthShader)["lightSpaceMat"] = lightSpaceMatrix;
	(*depthShader)["model"] = glm::mat4();
	depthShader->Use([&]() {
		blob->Render();
	});
	depthShader->Use([&]() {
		GLuint uMMatrix = depthShader->GetUniformLocation("model");
		level->Render(uMMatrix, -1);
	});

	glDisable(GL_CULL_FACE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderingManager::geometryPass(Level *level, glm::mat4 viewMatrix, glm::mat4 projMatrix)
{
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.FBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	(*geomPassShader)["projection"] = projMatrix;
	(*geomPassShader)["view"] = viewMatrix;
	GLuint uMMatrix = geomPassShader->GetUniformLocation("model");
	GLuint uColor = geomPassShader->GetUniformLocation("objectColor");
	geomPassShader->Use([&]() {
		level->Render(uMMatrix, uColor);
	});

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderingManager::SSAOPass(glm::mat4 projMatrix, glm::vec3 camPos)
{
	glViewport(0, 0, AO_DIM, AO_DIM);
	glBindFramebuffer(GL_FRAMEBUFFER, aoBuffer.FBO);
	glClear(GL_COLOR_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gBuffer.texture0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gBuffer.texture1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);

	(*SSAOShader)["screenSize"] = glm::vec2(AO_DIM, AO_DIM);
	SSAOShader->Use([&]() {
		quad->Draw();
	});

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderingManager::blurPass()
{
	GLboolean firstIteration = true, horizontal = true;
	GLuint amount = 4;
	for (GLuint i = 0; i < amount; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, pingpongBuffers[horizontal].FBO);
		glClear(GL_COLOR_BUFFER_BIT);
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, firstIteration ? aoBuffer.texture0 : pingpongBuffers[!horizontal].texture0);

		(*blurShader)["horizontal"] = horizontal;
		blurShader->Use([&]() {
			quad->Draw();
		});
	
		horizontal = !horizontal;
		if (firstIteration)  firstIteration = false;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);
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
	(*skyboxShader)["model"] = skybox.modelMat;
	(*skyboxShader)["view"] = viewMatrix;
	(*skyboxShader)["projection"] = projMatrix;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.getID());
	skyboxShader->Use([&]() {
		skybox.render();
	});

	glDepthFunc(GL_LESS);
}

void RenderingManager::debugQuadDraw()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthBuffer.texture0);
	quadShader->Use([&]() {
		quad->Draw();
	});
}