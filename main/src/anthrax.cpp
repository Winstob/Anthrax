/* ---------------------------------------------------------------- *\
 * anthrax.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-02-03
\* ---------------------------------------------------------------- */
#include <fstream>
#include "anthrax.hpp"

#ifndef WINDOW_NAME
#define WINDOW_NAME Anthrax
#endif

#ifndef FONT_DIRECTORY
#define FONT_DIRECTORY fonts
#endif


namespace Anthrax
{

// Static member allocation
unsigned int Anthrax::window_width_ = 0;
unsigned int Anthrax::window_height_ = 0;
bool Anthrax::window_size_changed_ = false;


Anthrax::Anthrax()
{
	window_width_ = 800;
	window_height_ = 600;
	/*
	int world_size = 4;
	world_ = new World(world_size);
	*/
	int world_size = 4096;
	world_ = new World(log2(world_size)/log2(1u<<LOG2K));

	// Load materials
	// TODO: make this less horrible
	materials_.clear();
	Material *tmp_ptr = world_->getMaterialsPtr();
	for (unsigned int i = 0; i < world_->getNumMaterials(); i++)
		materials_.push_back(tmp_ptr[i]);

	//camera_ = Camera(glm::vec3(pow(2, world_size-3), pow(2, world_size-3), 0.0));
	camera_ = Camera(glm::vec3(0.0, 0.0, 0.0));
	//camera_ = Camera(glm::ivec3(0, 0, 0));

	vulkan_manager_ = new VulkanManager();
}


Anthrax::~Anthrax()
{
	vulkan_manager_->wait();
	for (unsigned int i = 0; i < raymarched_images_.size(); i++)
		raymarched_images_[i].destroy();
	materials_staging_ssbo_.destroy();
	indirection_pool_staging_ssbo_.destroy();
	uniformity_pool_staging_ssbo_.destroy();
	voxel_type_pool_staging_ssbo_.destroy();
	materials_ssbo_.destroy();
	indirection_pool_ssbo_.destroy();
	uniformity_pool_ssbo_.destroy();
	voxel_type_pool_ssbo_.destroy();
	num_levels_ubo_.destroy();
	focal_distance_ubo_.destroy();
	screen_width_ubo_.destroy();
	screen_height_ubo_.destroy();
	camera_position_ubo_.destroy();
	camera_right_ubo_.destroy();
	camera_up_ubo_.destroy();
	camera_forward_ubo_.destroy();
	sunlight_ubo_.destroy();
	for (unsigned int i = 0; i < main_compute_descriptors_.size(); i++)
	{
		main_compute_descriptors_[i].destroy();
	}
	for (unsigned int i = 0; i < main_graphics_descriptors_.size(); i++)
	{
		main_graphics_descriptors_[i].destroy();
	}

	delete world_;
	delete vulkan_manager_;
	//exit();
}


int Anthrax::init()
{
	vulkan_manager_->setMultiBuffering(multibuffering_value_);
	vulkan_manager_->init();
	createBuffers();
	createDescriptors();

	vulkan_manager_->start();

	loadMaterials();
	createWorld();

	/*
	initializeShaders();
	textTexturesSetup();
	mainFramebufferSetup();
	textFramebufferSetup();

	createWorld();
	main_pass_shader_->use();
	initializeWorldSSBOs();

	*/

	return 0;
}


void Anthrax::setFOV(float angle)
{
	// Screen coordinates are between -1.0 and 1.0
	// Calculate the focal distance given a fov angle in degrees
	camera_.fov = angle;
	camera_.focal_distance = 1.0/tan(angle*0.5*PI/180.0);
	return;
}


void Anthrax::exit()
{
	/*
	delete main_pass_shader_;
	delete text_pass_shader_;
	delete screen_pass_shader_;
	glDeleteFramebuffers(1, &main_pass_framebuffer_);
	glDeleteTextures(1, &main_pass_texture_);
	glDeleteFramebuffers(1, &text_pass_framebuffer_);
	glDeleteBuffers(1, &indirection_pool_ssbo_);
	glDeleteBuffers(1, &voxel_type_pool_ssbo_);
	glDeleteBuffers(1, &lod_pool_ssbo_);
	*/

	/*
	vkDestroyInstance(vulkan_instance_, nullptr);
	glfwDestroyWindow(window_);
	glfwTerminate();
	*/
	return;
}


void Anthrax::renderFrame()
{
	updateCamera();
	
	// update buffers
	*((int*)num_levels_ubo_.getMappedPtr()) = world_->getNumLayers();
	*((float*)focal_distance_ubo_.getMappedPtr()) = camera_.focal_distance;
	*((int*)screen_width_ubo_.getMappedPtr()) = vulkan_manager_->getWindowWidth();
	*((int*)screen_height_ubo_.getMappedPtr()) = vulkan_manager_->getWindowHeight();
	// TODO: make this less bad
	Intfloat::vec3 camera_position {
		iComponents3(camera_.position) + glm::ivec3(std::pow(1 << LOG2K, world_->getNumLayers())/2),
		fComponents3(camera_.position)
	};
	*((Intfloat::vec3*)camera_position_ubo_.getMappedPtr()) = camera_position;
	*((glm::vec3*)camera_right_ubo_.getMappedPtr()) = camera_.getRightLookDirection();
	*((glm::vec3*)camera_up_ubo_.getMappedPtr()) = camera_.getUpLookDirection();
	*((glm::vec3*)camera_forward_ubo_.getMappedPtr()) = camera_.getForwardLookDirection();
	vulkan_manager_->drawFrame();

	/*
	if (window_size_changed_)
	{
		window_size_changed_ = false;
		mainFramebufferSetup();
		textFramebufferSetup();
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Main pass
	glBindFramebuffer(GL_FRAMEBUFFER, main_pass_framebuffer_);
	glClear(GL_COLOR_BUFFER_BIT);
	main_pass_shader_->use();
	main_pass_shader_->setInt("octree_layers", world_.num_layers_);
	main_pass_shader_->setFloat("focal_distance", camera_.focal_distance);
	main_pass_shader_->setInt("screen_width", window_width_);
	main_pass_shader_->setInt("screen_height", window_height_);
	main_pass_shader_->setIvec3("camera_position.int_component", iComponents3(camera_.position));
	main_pass_shader_->setVec3("camera_position.dec_component", fComponents3(camera_.position));
	main_pass_shader_->setVec3("camera_right", camera_.getRightLookDirection());
	main_pass_shader_->setVec3("camera_up", camera_.getUpLookDirection());
	main_pass_shader_->setVec3("camera_forward", camera_.getForwardLookDirection());
	//main_pass_shader_->setVec3("sunlight.direction", glm::normalize(glm::vec3(0.4, -0.5, 0.1)));
	main_pass_shader_->setVec3("sunlight.direction", glm::vec3(glm::cos(glfwGetTime()/16), -glm::sin(glfwGetTime()/16), glm::sin(glfwGetTime()/27)));
	main_pass_shader_->setVec3("sunlight.scatter_color", glm::normalize(glm::vec3(0.6, 0.6, 1.0)));
	main_pass_shader_->setVec3("sunlight.color", glm::normalize(glm::vec3(0.4, 0.4, 0.0)));
	renderFullscreenQuad();

	// Text pass
	glBindFramebuffer(GL_FRAMEBUFFER, text_pass_framebuffer_);
	glClear(GL_COLOR_BUFFER_BIT);
	text_pass_shader_->use();
	//renderText("Hi Bros", 25.0, static_cast<float>(window_height_)-45.0, 0.5, glm::vec3(1.0, 1.0, 1.0));
	for (unsigned int i = 0; i < texts_.size(); i++)
	{
		if (texts_.exists(i))
		renderText(texts_[i]);
	}

	// Screen pass
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	screen_pass_shader_->use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, main_pass_texture_);
	renderFullscreenQuad();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, text_pass_texture_);
	renderFullscreenQuad();


	glfwSwapBuffers(window_);
	glfwPollEvents();
	*/
	return;
}


void Anthrax::renderFullscreenQuad()
{
	/*
	if (quad_vao_ == 0)
	{
		// Set up fullscreen quad
		float quad_vertices_[] = {
			// positions  // texture coordinates
			-1.0, -1.0,   0.0, 0.0,
			1.0, -1.0,    1.0, 0.0,
			-1.0, 1.0,    0.0, 1.0,
			1.0, 1.0,     1.0, 1.0
		};
		glGenVertexArrays(1, &quad_vao_);
		glGenBuffers(1, &quad_vbo_);
		glBindVertexArray(quad_vao_);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vbo_);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices_), &quad_vertices_, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2*sizeof(float)));
		glEnableVertexAttribArray(1);
	}
	glBindVertexArray(quad_vao_);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
	*/
	return;
}


void Anthrax::initializeShaders()
{
	/*
	std::string shader_directory = xstr(SHADER_DIRECTORY);
	shader_directory += "/";

	main_pass_shader_ = new Shader(Shader::ShaderInputType::FILEPATH, (shader_directory + "main_pass_shaderv.glsl").c_str(), (shader_directory + "main_pass_shaderf.glsl").c_str());

	text_pass_shader_ = new Shader(Shader::ShaderInputType::FILEPATH, (shader_directory + "text_pass_shaderv.glsl").c_str(), (shader_directory + "text_pass_shaderf.glsl").c_str());

	screen_pass_shader_ = new Shader(Shader::ShaderInputType::FILEPATH, (shader_directory + "screen_pass_shaderv.glsl").c_str(), (shader_directory + "screen_pass_shaderf.glsl").c_str());
	*/

	return;
}


void Anthrax::loadMaterials()
{
	for (unsigned int i = 0; i < materials_.size(); i++)
	{
		((Material::PackedMaterial*)materials_staging_ssbo_.getMappedPtr())[i] = (materials_[i].pack());
	}
	materials_ssbo_.copy(materials_staging_ssbo_);
	return;
}


void Anthrax::createWorld()
{
	std::cout << "Copying world to staging buffers" << std::endl;
	memcpy(indirection_pool_staging_ssbo_.getMappedPtr(), world_->getIndirectionPool(), world_->getIndirectionPoolSize());
	memcpy(uniformity_pool_staging_ssbo_.getMappedPtr(), world_->getUniformityPool(), world_->getUniformityPoolSize());
	memcpy(voxel_type_pool_staging_ssbo_.getMappedPtr(), world_->getVoxelTypePool(), world_->getVoxelTypePoolSize());

	std::cout << "Moving world to local memory" << std::endl;
	indirection_pool_ssbo_.copy(indirection_pool_staging_ssbo_);
	uniformity_pool_ssbo_.copy(uniformity_pool_staging_ssbo_);
	voxel_type_pool_ssbo_.copy(voxel_type_pool_staging_ssbo_);
	std::cout << "Done!" << std::endl;
	return;
}


void Anthrax::initializeWorldSSBOs()
{
	/*
	glGenBuffers(1, &indirection_pool_ssbo_);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, indirection_pool_ssbo_);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 8 * sizeof(uint32_t) * world_.num_indices_, world_.indirection_pool_, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, indirection_pool_ssbo_);

	glGenBuffers(1, &voxel_type_pool_ssbo_);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxel_type_pool_ssbo_);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * world_.num_indices_, world_.voxel_type_pool_, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, voxel_type_pool_ssbo_);

	glGenBuffers(1, &lod_pool_ssbo_);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lod_pool_ssbo_);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * world_.num_indices_, world_.lod_pool_, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, lod_pool_ssbo_);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	*/
	return;
}


void Anthrax::updateCamera()
{
	/*
	camera_.position[0] = 10.0*glm::sin(glfwGetTime()/1.0);
	camera_.position[1] = 10.0*glm::cos(glfwGetTime()/1.0);
	*/

	// Update rotation
	float sensitivity = 0.01;
	glm::quat yaw_quaternion = glm::angleAxis(sensitivity*(-vulkan_manager_->getMouseMovementX()), camera_.getUpDirection());
	glm::quat pitch_quaternion = glm::angleAxis(sensitivity*(-vulkan_manager_->getMouseMovementY()), camera_.getRightLookDirection());
	camera_.rotation = glm::normalize(yaw_quaternion * pitch_quaternion * camera_.rotation);

	// Update position
	glm::vec3 motion_direction = glm::vec3(0.0, 0.0, 0.0);
	float walking_speed = 1.5; // meters per second
	float speed_multiplier = walking_speed * 100.0f; // Each voxel is 1cm in width
	float motion_multiplier = vulkan_manager_->getFrameTime() * speed_multiplier;

	//std::cout << vulkan_manager_->getKey(GLFW_KEY_W) << std::endl;
	if (vulkan_manager_->getKey(GLFW_KEY_A) == GLFW_PRESS)
	{
		motion_direction.x += -1.0;
	}
	if (vulkan_manager_->getKey(GLFW_KEY_D) == GLFW_PRESS)
	{
		motion_direction.x += 1.0;
	}
	if (vulkan_manager_->getKey(GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
	{
		motion_direction.y += -1.0;
	}
	if (vulkan_manager_->getKey(GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		motion_direction.y += 1.0;
	}
	if (vulkan_manager_->getKey(GLFW_KEY_W) == GLFW_PRESS)
	{
		motion_direction.z += -1.0;
	}
	if (vulkan_manager_->getKey(GLFW_KEY_S) == GLFW_PRESS)
	{
		motion_direction.z += 1.0;
	}

	glm::normalize(motion_direction);
	motion_direction *= motion_multiplier;
	motion_direction = camera_.rotation * motion_direction;

	camera_.position[0] += motion_direction.x;
	camera_.position[1] += motion_direction.y;
	camera_.position[2] += motion_direction.z;

	return;
}


void Anthrax::mainFramebufferSetup()
{
	/*
	if (main_pass_framebuffer_ == 0)
	{
		glGenFramebuffers(1, &main_pass_framebuffer_);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, main_pass_framebuffer_);

	if (main_pass_texture_ == 0)
	{
		glGenTextures(1, &main_pass_texture_);
	}

	glBindTexture(GL_TEXTURE_2D, main_pass_texture_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window_width_, window_height_, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, main_pass_texture_, 0);

	unsigned int attachment = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1, &attachment);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	std::cout << "Framebuffer not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	*/
	return;
}


void Anthrax::textTexturesSetup()
{
	/*
	FT_Library ft;
	if (FT_Init_FreeType(&ft))
	{
		std::cout << "ERROR::FREETYPE: Could not init FreeType library" << std::endl;
		return;
	}
	FT_Face face;
	std::string font_directory = xstr(FONT_DIRECTORY);
	//if (FT_New_Face(ft, (font_directory + "/Ubuntu-BI.ttf").c_str(), 0, &face))
	//if (FT_New_Face(ft, (font_directory + "/tmp.ttf").c_str(), 0, &face))
	if (FT_New_Face(ft, "fonts/UbuntuMono-R.ttf", 0, &face))
	{
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
		return;
	}
	FT_Set_Pixel_Sizes(face, 0, 64);
	glPixelStoref(GL_UNPACK_ALIGNMENT, 1);

	for (unsigned char c = 0; c < 128; c++)
	{
		// Load character glyph
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
			std::cout << "ERROR::FREETYPE: Failed to load glyph" << std::endl;
			continue;
		}
		// Generate texture
		unsigned int texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
		);
		// Set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// Store character info
		Character character = Character(
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			static_cast<unsigned int>(face->glyph->advance.x)
		);
		character_map_.insert(std::pair<GLchar, Character>(c, character));
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	if (text_vao_ == 0)
	{
		glGenVertexArrays(1, &text_vao_);
		glGenBuffers(1, &text_vbo_);
		glBindVertexArray(text_vao_);
		glBindBuffer(GL_ARRAY_BUFFER, text_vbo_);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*6*4, NULL, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4*sizeof(float), 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	*/

	return;
}

void Anthrax::textFramebufferSetup()
{
	/*
	if (text_pass_framebuffer_ == 0)
	{
		glGenFramebuffers(1, &text_pass_framebuffer_);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, text_pass_framebuffer_);

	if (text_pass_texture_ == 0)
	{
		glGenTextures(1, &text_pass_texture_);
	}

	glBindTexture(GL_TEXTURE_2D, text_pass_texture_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window_width_, window_height_, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, text_pass_texture_, 0);

	unsigned int attachment = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1, &attachment);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	*/
	return;
}


void Anthrax::renderText(std::string text, float x, float y, float scale, glm::vec3 color)
{
	/*
	text_pass_shader_->use();
	text_pass_shader_->setVec3("color", color);
	glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(window_width_), 0.0f, static_cast<float>(window_height_));
	text_pass_shader_->setMat4("projection", projection);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(text_vao_);

	// Iterate through all characters
	std::string::const_iterator c;
	for (c = text.begin(); c != text.end(); c++) 
	{
		Character ch = character_map_[*c];

		float xpos = x + ch.Bearing.x * scale;
		float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

		float w = ch.Size.x * scale;
		float h = ch.Size.y * scale;
		// Update VBO for each character
		float vertices[6][4] = {
			{ xpos,     ypos + h,   0.0f, 0.0f },            
			{ xpos,     ypos,       0.0f, 1.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },

			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },
			{ xpos + w, ypos + h,   1.0f, 0.0f }           
		};
		// Render glyph texture over quad
		glBindTexture(GL_TEXTURE_2D, ch.TextureID);
		// Update content of VBO memory
		glBindBuffer(GL_ARRAY_BUFFER, text_vbo_);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// Render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	*/

	return;
}


unsigned int Anthrax::addText(std::string text, float x, float y, float scale, glm::vec3 color)
{
	return texts_.add(Text(text, x, y, scale, color));
}


int Anthrax::removeText(unsigned int id)
{
	return texts_.remove(id);
}


void Anthrax::createBuffers()
{
	// ssbos
	materials_staging_ssbo_ = Buffer(
			vulkan_manager_->getDevice(),
			materials_.size() * sizeof(Material::PackedMaterial),
			Buffer::STORAGE_TYPE,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
	indirection_pool_staging_ssbo_ = Buffer(
			vulkan_manager_->getDevice(),
			world_->getIndirectionPoolSize(),
			Buffer::STORAGE_TYPE,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
	uniformity_pool_staging_ssbo_ = Buffer(
			vulkan_manager_->getDevice(),
			world_->getUniformityPoolSize(),
			Buffer::STORAGE_TYPE,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
	voxel_type_pool_staging_ssbo_ = Buffer(
			vulkan_manager_->getDevice(),
			world_->getVoxelTypePoolSize(),
			Buffer::STORAGE_TYPE,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);

	materials_ssbo_ = Buffer(
			vulkan_manager_->getDevice(),
			materials_.size() * sizeof(Material::PackedMaterial),
			Buffer::STORAGE_TYPE,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);
	indirection_pool_ssbo_ = Buffer(
			vulkan_manager_->getDevice(),
			world_->getIndirectionPoolSize(),
			Buffer::STORAGE_TYPE,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);
	uniformity_pool_ssbo_ = Buffer(
			vulkan_manager_->getDevice(),
			world_->getUniformityPoolSize(),
			Buffer::STORAGE_TYPE,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);
	voxel_type_pool_ssbo_ = Buffer(
			vulkan_manager_->getDevice(),
			world_->getVoxelTypePoolSize(),
			Buffer::STORAGE_TYPE,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);

	// ubos
	num_levels_ubo_= Buffer(
			vulkan_manager_->getDevice(),
			4,
			Buffer::UNIFORM_TYPE,
			0,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
	focal_distance_ubo_ = Buffer(
			vulkan_manager_->getDevice(),
			4,
			Buffer::UNIFORM_TYPE,
			0,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
	screen_width_ubo_ = Buffer(
			vulkan_manager_->getDevice(),
			4,
			Buffer::UNIFORM_TYPE,
			0,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
	screen_height_ubo_ = Buffer(
			vulkan_manager_->getDevice(),
			4,
			Buffer::UNIFORM_TYPE,
			0,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
	camera_position_ubo_ = Buffer(
			vulkan_manager_->getDevice(),
			32,
			Buffer::UNIFORM_TYPE,
			0,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
	camera_right_ubo_ = Buffer(
			vulkan_manager_->getDevice(),
			12,
			Buffer::UNIFORM_TYPE,
			0,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
	camera_up_ubo_ = Buffer(
			vulkan_manager_->getDevice(),
			12,
			Buffer::UNIFORM_TYPE,
			0,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
	camera_forward_ubo_ = Buffer(
			vulkan_manager_->getDevice(),
			12,
			Buffer::UNIFORM_TYPE,
			0,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
	sunlight_ubo_ = Buffer(
			vulkan_manager_->getDevice(),
			32,
			Buffer::UNIFORM_TYPE,
			0,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);

	for (unsigned int i = 0; i < multibuffering_value_; i++)
	{
		raymarched_images_.push_back(Image(vulkan_manager_->getDevice(),
				MAX_WINDOW_X,
				MAX_WINDOW_Y,
				VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
				));
	}

	return;
}


void Anthrax::createDescriptors()
{
	std::vector<Buffer> buffers;
	std::vector<Image> images;

	// main render pass
	buffers.clear();
	images.clear();
	buffers.resize(0);
	images.resize(1);
	main_graphics_descriptors_.clear();
	for (unsigned int i = 0; i < raymarched_images_.size(); i++)
	{
		images[0] = raymarched_images_[i];
		main_graphics_descriptors_.push_back(Descriptor(vulkan_manager_->getDevice(), Descriptor::ShaderStage::FRAGMENT, buffers, images));
	}
	vulkan_manager_->renderPassSetDescriptors(main_graphics_descriptors_);

	// main compute pass
	buffers.clear();
	images.clear();
	buffers.resize(13);
	images.resize(1);
	main_compute_descriptors_.clear();
	
	buffers[0] = materials_ssbo_;
	buffers[1] = indirection_pool_ssbo_;
	buffers[2] =  uniformity_pool_ssbo_;
	buffers[3] = voxel_type_pool_ssbo_;
	buffers[4] = num_levels_ubo_;
	buffers[5] = focal_distance_ubo_;
	buffers[6] = screen_width_ubo_;
	buffers[7] = screen_height_ubo_;
	buffers[8] = camera_position_ubo_;
	buffers[9] = camera_right_ubo_;
	buffers[10] = camera_up_ubo_;
	buffers[11] = camera_forward_ubo_;
	buffers[12] = sunlight_ubo_;
	for (unsigned int i = 0; i < raymarched_images_.size(); i++)
	{
		images[0] = raymarched_images_[i];
		main_compute_descriptors_.push_back(Descriptor(vulkan_manager_->getDevice(), Descriptor::ShaderStage::COMPUTE, buffers, images));
	}
	vulkan_manager_->computePassSetDescriptors(main_compute_descriptors_);
}

} // namespace Anthrax
