// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <float.h>
#include <iterator>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>

#include <algorithm>

#include "gmap.hpp"

std::ostream & operator<<(std::ostream &out, const glm::vec3 &v) {
  out << v.x << " " << v.y << " " << v.z;
  return out;
}

/*******************************************************************************/

int display(GMap3D& gmap)
{
  srand(time(NULL));
  // Initialise GLFW
  if( !glfwInit() )
    {
      fprintf( stderr, "Failed to initialize GLFW\n" );
      getchar();
      return -1;
    }

  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Open a window and create its OpenGL context
  window = glfwCreateWindow( 1024, 768, "TP3 - GMap", NULL, NULL);
  if( window == NULL ){
    fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
    getchar();
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  // Initialize GLEW
  glewExperimental = true; // Needed for core profile
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    getchar();
    glfwTerminate();
    return -1;
  }

  // Ensure we can capture the escape key being pressed below
  glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
  // Hide the mouse and enable unlimited mouvement
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  // Set the mouse at the center of the screen
  glfwPollEvents();
  glfwSetCursorPos(window, 1024/2, 768/2);

  // Dark blue background
  glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

  // Enable depth test
  glEnable(GL_DEPTH_TEST);
  // Accept fragment if it closer to the camera than the former one
  glDepthFunc(GL_LESS);

  // Cull triangles which normal is not towards the camera
  glEnable(GL_CULL_FACE);

  GLuint VertexArrayID;
  glGenVertexArrays(1, &VertexArrayID);
  glBindVertexArray(VertexArrayID);

  // Create and compile our GLSL program from the shaders
  GLuint programID = LoadShaders( "vertex_shader.glsl", "fragment_shader.glsl" );

  // Get a handle for our "MVP" uniform
  GLuint MatrixID = glGetUniformLocation(programID, "MVP");
  GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
  GLuint ModelMatrixID = glGetUniformLocation(programID, "M");


  std::vector<unsigned short> indices; //Triangles concaténés dans une liste
  std::vector<glm::vec3> indexed_vertices;
  std::vector<glm::vec3> indexed_normals;
  std::vector<unsigned int> property;

  // transform the gmap into set of triangles
  // TOCOMPLETE
  // for the property, you can first use a random value for each vertices.
    
  auto & ver = indexed_vertices;

  indexed_normals.resize(gmap.elements(0).size());
  
  for(id_t f : gmap.elements(2)) {
      
    std::cout << f << std::endl;
    GMap::idlist_t orbite = gmap.orderedorbit({0, 1}, f);

    //std::sort(orbite.begin(), orbite.end());
    //std::reverse(orbite.begin(), orbite.end());

    
    
    // Conserve les indices de la face en cours
    GMap::idlist_t indices_in_face;
    std::vector<vec3_t> vertex_in_face;
    
    for (size_t i = 0; i < orbite.size(); i+= 2) {
      id_t id = orbite.at(i);
	
      if (find(ver.begin(), ver.end(), gmap.get_position(id)) == ver.end()) {
	ver.push_back(gmap.get_position(id));
	indices_in_face.push_back(ver.size() - 1);
	vertex_in_face.push_back(gmap.get_position(id));
      }
      else {
	auto it = find(ver.begin(), ver.end(), gmap.get_position(id));
	int ind = std::distance(ver.begin(), it);
	indices_in_face.push_back(ind);
	vertex_in_face.push_back(*it);
      }
    }

    indices.push_back(indices_in_face.at(0));
    indices.push_back(indices_in_face.at(1));
    indices.push_back(indices_in_face.at(2));
    
    if (indices_in_face.size() == 4) {
      indices.push_back(indices_in_face.at(0));
      indices.push_back(indices_in_face.at(2));
      indices.push_back(indices_in_face.at(3));      
      
      vec3_t a = vertex_in_face[1] - vertex_in_face[0];
      vec3_t b = vertex_in_face[2] - vertex_in_face[0];
      vec3_t n = glm::normalize(glm::cross(a, b));
      
      indexed_normals[indices_in_face.at(0)] = n;
      indexed_normals[indices_in_face.at(1)] = n;
      indexed_normals[indices_in_face.at(2)] = n;
      indexed_normals[indices_in_face.at(3)] = n;
    }
    else if (indices_in_face.size() == 3) {
      
      vec3_t a = vertex_in_face[1] - vertex_in_face[0];
      vec3_t b = vertex_in_face[2] - vertex_in_face[0];
      vec3_t n = glm::normalize(glm::cross(a, b));
      
      indexed_normals[indices_in_face.at(0)] = n;
      indexed_normals[indices_in_face.at(1)] = n;
      indexed_normals[indices_in_face.at(2)] = n;
    }
  }
    
  for (size_t i = 0; i < indexed_vertices.size(); i++)
    {
      property.push_back(rand() % 100);
    }	
  
  glm::vec3 bb_min( FLT_MAX, FLT_MAX, FLT_MAX );
  glm::vec3 bb_max( FLT_MIN, FLT_MIN, FLT_MIN );

  //Calcul de la boite englobante du modèle
  for( unsigned int i = 0 ; i < indexed_vertices.size() ; i++ ){
    bb_min = glm::min(bb_min, indexed_vertices[i]);
    bb_max = glm::max(bb_max, indexed_vertices[i]);
  }

  glm::vec3 size = bb_max - bb_min;
  glm::vec3 center = glm::vec3(bb_min.x + size.x/2, bb_min.y + size.y/2 , bb_min.z + size.z/2 );
  float model_scale = 2.0/std::max( std::max(size.x, size.y), size.z );


  //****************************************************************/

  // Load it into a VBO

  GLuint vertexbuffer;
  glGenBuffers(1, &vertexbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
  glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);

  GLuint normalbuffer;
  glGenBuffers(1, &normalbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
  glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3), &indexed_normals[0], GL_STATIC_DRAW);

  GLuint propertybuffer;
  glGenBuffers(1, &propertybuffer);
  glBindBuffer(GL_ARRAY_BUFFER, propertybuffer);
  glBufferData(GL_ARRAY_BUFFER, property.size() * sizeof(float), &property[0], GL_STATIC_DRAW);

  // Generate a buffer for the indices as well
  GLuint elementbuffer;
  glGenBuffers(1, &elementbuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0] , GL_STATIC_DRAW);

  // Get a handle for our "LightPosition" uniform
  glUseProgram(programID);
  GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

  // For speed computation
  double lastTime = glfwGetTime();
  int nbFrames = 0;

  do{

    // Measure speed
    double currentTime = glfwGetTime();
    nbFrames++;
    if ( currentTime - lastTime >= 1.0 ){ // If last prinf() was more than 1sec ago
      // printf and reset
      printf("%f ms/frame\n", 1000.0/double(nbFrames));
      nbFrames = 0;
      lastTime += 1.0;
    }

    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use our shader
    glUseProgram(programID);

    // Projection matrix : 45 Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    glm::mat4 ProjectionMatrix = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
    // Camera matrix
    glm::mat4 ViewMatrix       = glm::lookAt(
					     glm::vec3(0,0,5), // Camera is at (4,3,3), in World Space
					     glm::vec3(0,0,0), // and looks at the origin
					     glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
					     );
    // Model matrix : an identity matrix (model will be at the origin)
    glm::mat4 ModelMatrix      = glm::scale(glm::mat4(1.0f), glm::vec3(model_scale))*glm::translate(glm::mat4(1.0f), glm::vec3(-center.x, -center.y, -center.z));
    ModelMatrix = glm::rotate(ModelMatrix, 0.5f * (float)currentTime, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

    // Send our transformation to the currently bound shader,
    // in the "MVP" uniform
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
    glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

    glm::vec3 lightPos = glm::vec3(4,4,4);
    glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);

    // 1rst attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(
			  0,                  // attribute
			  3,                  // size
			  GL_FLOAT,           // type
			  GL_FALSE,           // normalized?
			  0,                  // stride
			  (void*)0            // array buffer offset
			  );


    // 3rd attribute buffer : normals
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
    glVertexAttribPointer(
			  1,                                // attribute
			  3,                                // size
			  GL_FLOAT,                         // type
			  GL_FALSE,                         // normalized?
			  0,                                // stride
			  (void*)0                          // array buffer offset
			  );

    // 4th attribute buffer : property
    glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, propertybuffer);
    glVertexAttribPointer(
			  2,                                // attribute
			  1,                                // size
			  GL_FLOAT,                         // type
			  GL_FALSE,                         // normalized?
			  0,                                // stride
			  (void*)0                          // array buffer offset
			  );

    // Index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

    // Draw the triangles !
    glDrawElements(
		   GL_TRIANGLES,      // mode
		   indices.size(),    // count
		   GL_UNSIGNED_SHORT,   // type
		   (void*)0           // element array buffer offset
		   );

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    // Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();

  } // Check if the ESC key was pressed or the window was closed
  while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
	 glfwWindowShouldClose(window) == 0 );

  // Cleanup VBO and shader
  glDeleteBuffers(1, &vertexbuffer);
  glDeleteBuffers(1, &normalbuffer);
  glDeleteBuffers(1, &propertybuffer);
  glDeleteBuffers(1, &elementbuffer);
  glDeleteProgram(programID);
  glDeleteVertexArrays(1, &VertexArrayID);

  // Close OpenGL window and terminate GLFW
  glfwTerminate();

  return 0;
}

