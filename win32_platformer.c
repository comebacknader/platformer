#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <windows.h>
#include <stdio.h>
#include "platformer.h"

#define GL_LITE_IMPLEMENTATION
#include "gl_lite.h"

#define HANDMADE_MATH_USE_DEGREES
#include "HandmadeMath.h"

/*

	- Start with basic shapes and color - this means take off textures, just draw shapes with colors
	- Populate the world with rectangles
	- Have a player rectangle sit on ground rectangle, move and jump
	- Have player accelerate when holding a button 
	- Create slopes in the world, and have player walk up and down slopes
	- Have platforms that the character can jump from bottom into 
	- Have character be able to wall slide, and jump off walls

	Things I Want:
	- ALL 2D Lighting: Glow, Normal Maps, etc. 
	- Particle Effects

*/

global b32 game_loop;
global HGLRC rendering_context;
global float SCREEN_WIDTH = 1280.0;
global float SCREEN_HEIGHT = 720.0;
global i64 global_performance_counter_frequency;
global f64 ms_per_frame;
global i64 counter_elapsed;

HMM_Vec3 camera_position;
HMM_Vec3 camera_front;
HMM_Vec3 up;
HMM_Vec3 camera_target;
HMM_Vec3 camera_direction;
HMM_Vec3 camera_right;
HMM_Vec3 camera_up;

typedef struct FileReadResults
{
	u32 contents_size;
	void* contents;
} FileReadResults;

global void
console_print_f32(char* fmt_string, f32 number)
{
	char buffer[1024];
	snprintf(buffer, sizeof(buffer), fmt_string, number);
	OutputDebugStringA(buffer);
}

global char *
concat_strings(char *str1, char *str2) 
{
	u32 str1_length = 0;
	for (int x = 0; str1[x] != '\0'; x++) {
		str1_length++;
	}
	u32 str2_length = 0;
	for (int x = 0; str2[x] != '\0'; x++) {
		str2_length++;
	}
	u32 new_string_length = str1_length + str2_length;
	char* new_string = (char*)malloc(sizeof(char) * new_string_length);
	u32 index = 0;
	for (int x = 0; x < str1_length; x++)
	{
		new_string[index] = str1[x];
		index++;
	}
	for (int x = 0; x < str2_length; x++)
	{
		new_string[index] = str2[x];
		index++;
	}
	return new_string;
}

global u32
get_length(char* str)
{

}

internal u8
strings_equal(char* str1, char* str2)
{
	int str1_length = 0;
	int str2_length = 0;
	for (int x = 0; str1[x] != '\0'; x++)
	{
		str1_length++;
	}
	for (int x = 0; str2[x] != '\0'; x++)
	{
		str2_length++;
	}
	if (str1_length != str2_length)
	{
		return(0);
	}
	else
	{
		for (int x = 0; x < str1_length; x++)
		{
			if (str1[x] != str2[x])
			{
				return(0);
			}
		}
		return(1);
	}

}

internal LRESULT CALLBACK
win32_main_window_callback(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	LRESULT result = 0;
	switch (message)
	{
	case WM_CLOSE:
	{
		game_loop = false;
	} break;
	case WM_ACTIVATEAPP:
	{

	} break;
	case WM_DESTROY:
	{

	} break;
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	{} break;
	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC device_context = BeginPaint(window, &paint);
		EndPaint(window, &paint);
	} break;
	default:
	{
		result = DefWindowProcA(window, message, wparam, lparam);
	} break;
	}
	return(result);
}

internal void
win32_init_opengl(HWND window)
{
	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,
		8,
		0,
		PFD_MAIN_PLANE,
		0, 0, 0
	};

	HDC window_dc = GetDC(window);
	int pixel_format = ChoosePixelFormat(window_dc, &pfd);
	SetPixelFormat(window_dc, pixel_format, &pfd);
	rendering_context = wglCreateContext(window_dc);
	if (wglMakeCurrent(window_dc, rendering_context))
	{
		// TODO(Nader): Log
	}
	else
	{
		OutputDebugStringA("Failed to make current openGL rendering context \n");
	}
	int initialized_opengl = gl_lite_init();
	if (!initialized_opengl)
	{
		OutputDebugStringA("Failed to dynamically load function pointers \n");
	}
	else
	{
		OutputDebugStringA("Successfully loaded function pointers \n");
	}
	ReleaseDC(window, window_dc);
}

inline u32
truncate_u64(u64 value)
{
	u32 result = (u32)value;
	return(result);
}

internal void
free_file_memory(void* file_memory)
{
	if (file_memory)
	{
		VirtualFree(file_memory, 0, MEM_RELEASE);
	}
}

internal FileReadResults
read_file_to_memory(char* filepath)
{
	FileReadResults result = { 0 };
	HANDLE file_handle = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (file_handle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER file_size;
		if (GetFileSizeEx(file_handle, &file_size))
		{
			u32 file_size_32 = truncate_u64(file_size.QuadPart);
			result.contents = VirtualAlloc(0, file_size_32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (result.contents)
			{
				DWORD bytes_read;
				if (ReadFile(file_handle, result.contents, file_size_32, &bytes_read, 0)
					&& (file_size_32 == bytes_read))
				{
					OutputDebugStringA("File Read successfully \n");
					result.contents_size = file_size_32;
				}
				else
				{
					if (result.contents)
					{
						free_file_memory(result.contents);
					}
				}
			}
		}
		else
		{
			// TODO(Nader): Logging 
		}
		CloseHandle(file_handle);
	}
	return(result);
}

internal void
check_shader_errors(char* type, u32 object)
{
	int success;
	char info_log[512];
	if (!strings_equal("PROGRAM", type))
	{
		glGetShaderiv(object, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(object, 512, NULL, info_log);
			char temp[1024];
			if (strings_equal("VERTEX", type))
			{
				wsprintfA(temp, "ERROR::SHADER::VERTEX::COMPILATION::FAILED - %s \n", info_log);
			}
			else
			{
				wsprintfA(temp, "ERROR::SHADER::FRAGMENT::COMPILATION::FAILED - %s \n", info_log);
			}
			OutputDebugStringA(temp);
		}
		else
		{
			if (strings_equal("VERTEX", type))
			{
				OutputDebugStringA("Vertex Shader compiled successfully. \n");
			}
			else
			{
				OutputDebugStringA("Fragment Shader compiled successfully. \n");
			}
		}
	}
	else
	{
		glGetProgramiv(object, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(object, 512, NULL, info_log);
			char temp[1024];
			wsprintfA(temp, "ERROR::SHADER::PROGRAM::LINKING::FAILED - %s \n", info_log);
			OutputDebugStringA(temp);
		}
		else
		{
			OutputDebugStringA("Shader Program linked successfully. \n");
		}
	}
}

internal void
win32_process_pending_messages(GameControllerInput* keyboard_controller)
{
	MSG message = { 0 };
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		switch (message.message)
		{
		case WM_QUIT:
		{
			game_loop = false;
			wglDeleteContext(rendering_context);
		} break;
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			u32 vk_code = (u32)message.wParam;
			b32 was_down = ((message.lParam & (1 << 30)) != 0);
			b32 is_down = ((message.lParam & (1 << 31)) == 0);

			if (true)
			{
				f64 camera_speed = 0.005f * counter_elapsed;
				console_print_f32("counter_elapsed: %.05f \n", counter_elapsed);
				console_print_f32("camera_speed: %.05f \n", camera_speed);

				if (vk_code == VK_ESCAPE)
				{
					game_loop = false;
				}
				else if (vk_code == VK_UP)
				{
					camera_position = HMM_AddV3(camera_position, HMM_MulV3F(camera_front, camera_speed));
				}
				else if (vk_code == VK_DOWN)
				{
					camera_position = HMM_SubV3(camera_position, HMM_MulV3F(camera_front, camera_speed));
				}
				else if (vk_code == VK_LEFT)
				{
					camera_position = HMM_SubV3(camera_position, HMM_NormV3(camera_front, camera_speed));
				}
			}
		} break;
		default:
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		} break;
		}
	}
}

int CALLBACK
WinMain(HINSTANCE instance,
	HINSTANCE prev_instance,
	LPSTR command_line,
	int show_code)
{
	LARGE_INTEGER performance_counter_frequency;
	QueryPerformanceFrequency(&performance_counter_frequency);
	global_performance_counter_frequency = performance_counter_frequency.QuadPart;

	WNDCLASSA window_class = { 0 };
	window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	window_class.lpfnWndProc = win32_main_window_callback;
	window_class.hInstance = instance;
	window_class.lpszClassName = "PlatformerWindowClass";
	window_class.hCursor = LoadCursor(0, IDC_ARROW);

	if (RegisterClassA(&window_class))
	{
		HWND window = CreateWindowExA(
			0, window_class.lpszClassName, "Platformer",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);

		if (window)
		{
			win32_init_opengl(window);
			stbi_set_flip_vertically_on_load(true);

			char* sprite_vertex_filepath = "G:\\VisualStudioProjects\\Platformer\\Platformer\\sprite_shader.vert";
			char* sprite_fragment_filepath = "G:\\VisualStudioProjects\\Platformer\\Platformer\\sprite_shader.frag";
			FileReadResults sprite_vertex_file = read_file_to_memory(sprite_vertex_filepath);
			FileReadResults sprite_fragment_file = read_file_to_memory(sprite_fragment_filepath);

			char* vertex_shader_source = (char*)sprite_vertex_file.contents;
			char* fragment_shader_source = (char*)sprite_fragment_file.contents;

			u32 vertex_shader;
			vertex_shader = glCreateShader(GL_VERTEX_SHADER);
			glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
			glCompileShader(vertex_shader);

			u32 fragment_shader;
			fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
			glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
			glCompileShader(fragment_shader);

			check_shader_errors("VERTEX", vertex_shader);
			check_shader_errors("FRAGMENT", fragment_shader);

			u32 shader_program;
			shader_program = glCreateProgram();
			glAttachShader(shader_program, vertex_shader);
			glAttachShader(shader_program, fragment_shader);
			glLinkProgram(shader_program);

			check_shader_errors("PROGRAM", shader_program);

			glDeleteShader(vertex_shader);
			glDeleteShader(fragment_shader);

			f32 vertices[] =
			{
				// position coordinates
				 1.0f,  1.0f, 0.0,		// top right
				 1.0f, -1.0f, 0.0,		// bottom right
				-1.0f, -1.0f, 0.0,		// bottom left
				-1.0f,  1.0f, 0.0,		// top left
			};

			 u32 indices[] =
			{
				0, 1, 3, // first triangle
				1, 2, 3  // second triangle
			};

			u32 vao, vbo, ebo;

			glGenVertexArrays(1, &vao);
			glGenBuffers(1, &vbo);
			glGenBuffers(1, &ebo);

			glBindVertexArray(vao);

			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);

			int monitor_refresh_hz = 60;
			HDC refresh_dc = GetDC(window);
			int win32_refresh_rate = GetDeviceCaps(refresh_dc, VREFRESH);
			ReleaseDC(window, refresh_dc);
			if (win32_refresh_rate > 1)
			{
				monitor_refresh_hz = win32_refresh_rate;
			}
			f32 game_update_hz = (monitor_refresh_hz / 2.0); // 30 frames 
			f32 target_seconds_per_frame = 1.0 / game_update_hz; // 1/30  1 second / 30 frames, 30 fps 

			game_loop = true;

			GameInput input[2] = { 0, 0 };
			GameInput *new_input = &input[0];
			GameInput *old_input = &input[1];

			camera_position = HMM_V3(0.0f, 0.0f, 3.0f);
			camera_front = HMM_V3(0.0f, 0.0f, -1.0f);
			up = HMM_V3(0.0f, 1.0f, 0.0f);

			camera_target = HMM_V3(0.0f, 0.0f, 0.0f);
			camera_direction = HMM_NormV3(HMM_SubV3(camera_position, camera_target));
			camera_right = HMM_NormV3(HMM_Cross(up, camera_direction));
			camera_up = HMM_Cross(camera_direction, camera_right);

			float delta_time = 0.0f;
			float last_time = 0.0f;

			// BEGIN FRAME

			LARGE_INTEGER last_counter;
			QueryPerformanceCounter(&last_counter);
			u64 last_cycle_count = __rdtsc();

			while (game_loop)
			{
				GameControllerInput* old_keyboard_controller = get_controller(old_input, 0);
				GameControllerInput* new_keyboard_controller = get_controller(new_input, 0);
				new_keyboard_controller->is_connected = true;
				for (int button_index = 0;
					button_index < array_count(new_keyboard_controller->buttons);
					++button_index)
				{
					new_keyboard_controller->buttons[button_index] =
						old_keyboard_controller->buttons[button_index];
				}

				win32_process_pending_messages(new_keyboard_controller);

				new_input->dt_for_frame = target_seconds_per_frame;
				HDC window_device_context = GetDC(window);
				RECT client_rect;
				GetClientRect(window, &client_rect);
				int window_width = client_rect.right - client_rect.left;
				int window_height = client_rect.bottom - client_rect.top;

				glViewport(0, 0, window_width, window_height);
				glClearColor(0.3f, 0.5f, 0.7f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);

				glUseProgram(shader_program);

				HMM_Mat4 view = HMM_M4D(1.0f);
				view = HMM_LookAt_RH(camera_position, HMM_AddV3(camera_position, camera_front), camera_up);

				// When I want to draw a new rectangle, I need to change the model of it
				HMM_Mat4 model = HMM_M4D(1.0f);
				model = HMM_Scale(HMM_V3(100.0f, 100.0f, 1.0f));
				model.Columns[3].X = 100.0f;
				model.Columns[3].Y = 100.0f;

				HMM_Mat4 projection = HMM_M4D(1.0f);
				projection = HMM_Orthographic_RH_ZO(0.0f, (f32)(window_width), 0.0f, (f32)(window_height), -1000.0f, 1000.0f);

				u32 model_location = glGetUniformLocation(shader_program, "model");
				u32 view_location = glGetUniformLocation(shader_program, "view");
				u32 projection_location = glGetUniformLocation(shader_program, "projection");

				glUniformMatrix4fv(model_location, 1, GL_FALSE, &model.Elements[0][0]);
				glUniformMatrix4fv(view_location, 1, GL_FALSE, &view.Elements[0][0]);
				glUniformMatrix4fv(projection_location, 1, GL_FALSE, &projection.Elements[0][0]);

				glBindVertexArray(vao);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

				model = HMM_M4D(1.0f);
				model = HMM_Scale(HMM_V3(100.0f, 100.0f, 1.0f));
				
				// Translation
				model.Columns[3].X = 500.0f;
				model.Columns[3].Y = 100.0f;
				model.Columns[3].Z = 0.0f;

				model_location = glGetUniformLocation(shader_program, "model");
				glUniformMatrix4fv(model_location, 1, GL_FALSE, &model.Elements[0][0]);
				
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

				model = HMM_M4D(1.0f);
				model = HMM_Scale(HMM_V3(100.0f, 100.0f, 1.0f));

				// Translation
				model.Columns[3].X = 500.0f;
				model.Columns[3].Y = 500.0f;
				model.Columns[3].Z = 1.0f;

				model_location = glGetUniformLocation(shader_program, "model");
				glUniformMatrix4fv(model_location, 1, GL_FALSE, &model.Elements[0][0]);

				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

				SwapBuffers(window_device_context);
				ReleaseDC(window, window_device_context);

				// END FRAME

				LARGE_INTEGER end_counter;
				QueryPerformanceCounter(&end_counter);

				u64 end_cycle_count = __rdtsc();
				u64 cycles_elapsed = end_cycle_count - last_cycle_count;
				last_cycle_count = end_cycle_count;

				counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;
				f64 fps = (f64)global_performance_counter_frequency / (f64)counter_elapsed;
				ms_per_frame = (((1000.0f * (f64)counter_elapsed / (f64)global_performance_counter_frequency)));
#if 0
				char buffer[1024];
				snprintf(buffer, sizeof(buffer), "%.02f fps | %.02f ms/f\n", fps, ms_per_frame);
				OutputDebugStringA(buffer);
#endif
				last_counter = end_counter;
				last_cycle_count = end_cycle_count;

			}
			glDeleteVertexArrays(1, &vao);
			glDeleteBuffers(1, &vbo);
			glDeleteBuffers(1, &ebo);
		}
		else
		{
			OutputDebugStringA("Could not create window \n");
		}
	}
	else
	{
		OutputDebugStringA("Could not register window class \n");
	}
}

#if 0
u32 texture1;
glGenTextures(1, &texture1);
glBindTexture(GL_TEXTURE_2D, texture1);

glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

int width, height, number_of_channels;
unsigned char* data = stbi_load(
	"G:\\VisualStudioProjects\\Platformer\\Platformer\\asset_packs\\Asset Pack-V1\\Player Idle\\Player Idle 48x48.png",
	&width, &height, &number_of_channels, 0);

if (data)
{
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
}
stbi_image_free(data);

glUseProgram(shader_program);
glUniform1i(glGetUniformLocation(shader_program, "texture1"), 0);
#endif