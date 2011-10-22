#include "Heatwave.hpp"
#include "gl/VertexArrayObject.hpp"
#include "gl/BufferObject.hpp"
#include "gl/Shader.hpp"
#include "gl/ShaderProgram.hpp"
#include "gl/Framebuffer.h"
#include "math/Matrix.hpp"
#include "math/MatrixTransform.hpp"
#include "math/Quaternion.hpp"
#include "math/misc.hpp"
#include "mesh/Material.hpp"
#include "mesh/VertexFormats.hpp"
#include "mesh/GPUMesh.hpp"
#include "mesh/ObjLoader.hpp"
#include "mesh/TextureManager.hpp"
#include "scene/Scene.hpp"
#include "scene/RenderContext.hpp"

#include <iostream>
#include <fstream>

#include "gl3w.hpp"

//#define GLFW_GL3_H
#include <GL/glfw.h>

void APIENTRY debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
{
	if ((type != GL_DEBUG_TYPE_PERFORMANCE_ARB && type != GL_DEBUG_TYPE_OTHER_ARB) || severity != GL_DEBUG_SEVERITY_LOW_ARB)
		std::cerr << message << std::endl;
	if ((type != GL_DEBUG_TYPE_PERFORMANCE_ARB && type != GL_DEBUG_TYPE_OTHER_ARB) || severity == GL_DEBUG_SEVERITY_HIGH_ARB)
		__asm int 3; // Breakpoint
}

bool init_window()
{
	if (glfwInit() != GL_TRUE)
	{
		char tmp;
		std::cerr << "Failed to initialize GLFW." << std::endl;
		std::cin >> tmp;
		return false;
	}

	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 3);
	glfwOpenWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwOpenWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	if (glfwOpenWindow(800, 600, 8, 8, 8, 0, 0, 0, GLFW_WINDOW) != GL_TRUE)
	{
		char tmp;
		std::cerr << "Failed to open window." << std::endl;
		std::cin >> tmp;
		return false;
	}

	glfwSwapInterval(1);

	if (gl3wInit() != 0) {
		char tmp;
		std::cerr << "Failed to initialize gl3w." << std::endl;
		std::cin >> tmp;
		return false;
	} else if (!gl3wIsSupported(3, 3)) {
		char tmp;
		std::cerr << "OpenGL 3.3 not supported." << std::endl;
		std::cin >> tmp;
		return false;
	}

	if (glDebugMessageCallbackARB) {
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
		glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
		glDebugMessageCallbackARB(debug_callback, 0);
	}

	glfwDisable(GLFW_MOUSE_CURSOR);

	return true;
}

struct MatUniforms : public MaterialUniforms {
	float dummy;
};

struct ShadingUniforms : public MaterialUniforms {
	float dummy;
};

int main(int argc, char *argv[])
{
	if (!init_window())
		return 1;

	{
		scene::RenderContext render_context(800, 600);
		scene::Scene scene;

		int mesh_id;
		{
			int mat_id;
			{
				Material material;
				material.loadFromFiles("test.vert", "test.frag");
				material.setOptionsSize(sizeof(MatUniforms));

				mat_id = scene.addMaterial(std::move(material));
			}

			GPUMesh mesh;

			std::ifstream objf("data/panel_beams.obj");
			Mesh base_mesh = load_obj(objf);
			auto& vertices = base_mesh.sub_meshes[0].vertices;
			auto& indices = base_mesh.sub_meshes[0].indices;
			mesh.loadVertexData(vertices.data(), vertices.size() * sizeof(vertex_fmt::Pos3f_Norm3f_Tex2f), vertex_fmt::FMT_POS3F_NORM3F_TEX2F);
			mesh.loadIndices(indices.data(), indices.size());

			mesh.material_id = mat_id;

			{
				auto u = std::make_shared<MatUniforms>();
				u->dummy = 1.f;

				MaterialOptions mtl_options;
				mtl_options.uniforms = std::move(u);
				mtl_options.texture_ids.fill(-1);
				mtl_options.texture_ids[0] = scene.tex_manager.loadTexture("panel_beams_diffuse.png");
				mtl_options.texture_ids[1] = scene.tex_manager.loadTexture("panel_beams_normal.png");
				mesh.material_options = mtl_options;
			}

			mesh_id = scene.addMesh(std::move(mesh));
		}

		scene::MeshInstance& inst = scene.newInstance(mesh_id);
		inst.pos_scale = math::vec4(0.f, 0.f, 0.f, 1.f);
		inst.rot = math::Quaternion();

		scene::Camera camera;
		camera.fov = 45.f;
		camera.clip_near = 0.1f;
		camera.clip_far = 500.f;
		camera.pos = math::vec3(0.f, 0.f, -5.f);
		camera.rot = math::Quaternion();

		{
			Material shading_material;
			shading_material.loadFromFiles("shading.vert", "shading.frag");
			shading_material.setOptionsSize(sizeof(ShadingUniforms));

			bool running = true;

			glClearColor(0.2f, 0.2f, 0.2f, 1.f);
			//glEnable(GL_DEPTH_TEST);
			//glEnable(GL_CULL_FACE);
			//glEnable(GL_DEPTH_CLAMP);
			glFrontFace(GL_CW);
			//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			scene::RenderBufferSet def_buffers;
			def_buffers.initialize(800, 600);

			/*
			std::cout <<
				"Teclas:\n"
				"  WASD - Movimentacao\n"
				"  R/F - Subir/descer\n"
				"  OKL; - Rotaciona camera\n"
				"\n"
				"  1/2 - Mudar nivel de detalhe fixo.\n" << std::endl;
			*/

			double elapsed_game_time = 0.;
			double elapsed_real_time = 0.;
			double last_frame_time;

			//bool prev_keys[2] = {false};

			math::Quaternion rot_amount(math::up, math::pi);

			int last_mouse_pos[2];
			glfwGetMousePos(&last_mouse_pos[0], &last_mouse_pos[1]);

			while (running)
			{
				double frame_start = glfwGetTime();

				int i;
				for (i = 0; elapsed_game_time < elapsed_real_time && i < 5; ++i)
				{
					/*
					if (glfwGetKey('O')) cam_rot -= vec3(0.02f, 0.f, 0.f); // Y - O
					if (glfwGetKey('L')) cam_rot += vec3(0.02f, 0.f, 0.f); // I - L
					if (glfwGetKey('K')) cam_rot -= vec3(0.f, 0.02f, 0.f); // E - K
					if (glfwGetKey(';')) cam_rot += vec3(0.f, 0.02f, 0.f); // O - ;

					// Inverse of view_rot below
					const mat3x4 view_rot_move = concatTransform(mat_transform::rotate(vec3(0.f, 1.f, 0.f), -cam_rot.getY()), mat_transform::rotate(vec3(1.f, 0.f, 0.f), -cam_rot.getX()));

					vec3 x_axis = transform(view_rot_move, vec3(1.f, 0.f, 0.f));
					vec3 y_axis = transform(view_rot_move, vec3(0.f, 1.f, 0.f));
					vec3 z_axis = transform(view_rot_move, vec3(0.f, 0.f, 1.f));

					if (glfwGetKey('A')) (*pos) -= x_axis * 0.25f; // A - A
					if (glfwGetKey('D')) (*pos) += x_axis * 0.25f; // S - D
					if (glfwGetKey('R')) (*pos) += y_axis * 0.25f; // P - R
					if (glfwGetKey('F')) (*pos) -= y_axis * 0.25f; // T - F
					if (glfwGetKey('W')) (*pos) += z_axis * 0.25f; // W - W
					if (glfwGetKey('S')) (*pos) -= z_axis * 0.25f; // R - S

					bool key = glfwGetKey('1') != 0;
					if (key && !prev_keys[0])
					{
						if (lod_level > 0)
							glUniform1i(u_LodLevel, --lod_level);
					}
					prev_keys[0] = key;

					key = glfwGetKey('2') != 0;
					if (key && !prev_keys[1])
					{
						//if (lod_level > 0)
						glUniform1i(u_LodLevel, ++lod_level);
					}
					prev_keys[1] = key;
					*/

					static const float ROT_SPEED = 0.02f;
					static const float MOUSE_ROT_SPEED = 0.01f;

					if (glfwGetKey('A')) rot_amount = math::Quaternion(math::up, ROT_SPEED) * rot_amount;
					if (glfwGetKey('D')) rot_amount = math::Quaternion(math::up, -ROT_SPEED) * rot_amount;

					if (glfwGetKey('W')) rot_amount = math::Quaternion(math::right, ROT_SPEED) * rot_amount;
					if (glfwGetKey('S')) rot_amount = math::Quaternion(math::right, -ROT_SPEED) * rot_amount;

					if (glfwGetKey('Q')) rot_amount = math::Quaternion(math::forward, ROT_SPEED) * rot_amount;
					if (glfwGetKey('E')) rot_amount = math::Quaternion(math::forward, -ROT_SPEED) * rot_amount;

					int cur_mouse_pos[2];
					glfwGetMousePos(&cur_mouse_pos[0], &cur_mouse_pos[1]);

					float x_mdelta = float(cur_mouse_pos[0] - last_mouse_pos[0]);
					float y_mdelta = float(cur_mouse_pos[1] - last_mouse_pos[1]);
					rot_amount = math::Quaternion(math::up, -x_mdelta * MOUSE_ROT_SPEED) * rot_amount;
					rot_amount = math::Quaternion(math::right, -y_mdelta * MOUSE_ROT_SPEED) * rot_amount;

					last_mouse_pos[0] = cur_mouse_pos[0];
					last_mouse_pos[1] = cur_mouse_pos[1];

					inst.rot = rot_amount;
					//view = math::concatTransform(math::mat_transform::translate3x4(math::vec3(0.f, 0.f, 5.f)), math::matrixFromQuaternion(rot_amount));

					elapsed_game_time += 1./60.;
				}

				//const mat3x4 view_rot = concatTransform(mat_transform::rotate(vec3(1.f, 0.f, 0.f), cam_rot.getX()), mat_transform::rotate(vec3(0.f, 1.f, 0.f),  cam_rot.getY()));
				//view = concatTransform(view_rot, mat_transform::translate3x4(cam_pos * -1.f));
				//view = mat_transform::look_at(make_vec(0.f, 1.f, 0.f), cam_pos, obj_pos);

				/*
				Light light_transformed;
				{
					light_transformed.color = lights.color;
					light_transformed.direction = vec3(transform(view, vec4(lights.direction)));
				}
				*/

				scene::renderGeometry(scene, camera, def_buffers, render_context);
				scene::shadeBuffers(scene.lights, shading_material, def_buffers, 0, render_context);

				glfwSwapBuffers();

				double tmp = elapsed_real_time + (glfwGetTime() - frame_start);
				if (elapsed_game_time > tmp)
					glfwSleep(elapsed_game_time - tmp - 0.01);

				last_frame_time = glfwGetTime() - frame_start;
				elapsed_real_time += last_frame_time;
				if (i == 5)
					elapsed_game_time = elapsed_real_time;

				running = glfwGetWindowParam(GLFW_OPENED) != 0 && glfwGetKey(GLFW_KEY_ESC) != GLFW_PRESS;
			}
		}
	}

	glfwTerminate();

	return 0;
}
