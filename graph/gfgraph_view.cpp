// Copyright (C) Intel Corp.  2014.  All Rights Reserved.

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice (including the
// next paragraph) shall be included in all copies or substantial
// portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

//  **********************************************************************/
//  * Authors:
//  *   Mark Janes <mark.a.janes@intel.com>
//  **********************************************************************/

#include "graph/gfgraph_view.h"

#include <assert.h>
#include <math.h>

// #include <iostream>
#include <QtOpenGL>
#include <GLES2/gl2.h>

#include <map>
#include <vector>

using Grafips::GraphViewRenderer;
using Grafips::GraphView;

#define GL_CHECK() CheckError(__FILE__, __LINE__)

const char *
GraphViewRenderer::vshader = "attribute vec2 coord2d;\n"
    "uniform float x_range; /* max milliseconds to display */\n"
    "uniform float max_y; /* max y value */\n"
    "void main(void) {\n"
    "  /* map the x value (ms age) into the range -1 <-> 1 */\n"
    "  float x = -2.0 * (coord2d.x / x_range) + 1.0;\n"
    "  /* map the x value (data point) into the range -1 <-> 1 */\n"
    "  float y = -2.0 * (coord2d.y / max_y) + 1.0;\n"
    "  gl_Position = vec4(x, y, 0, 1);\n"
    "}";

const char *
GraphViewRenderer::fshader = "uniform vec4 line_color;"
    "void main(void) {"
    "   gl_FragColor = line_color;"
    "}";

float tick_lines_color[4] = { 0, 0, 0, .2 };
float black_color[4] = { 0, 0, 0, 1 };

GraphViewRenderer::GraphViewRenderer(GraphSetSubscriber *s,
                                     const PublisherInterface &p)
    : m_subscriber(s) {
  glGenBuffers(1, &vbo);
  GL_CHECK();
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  GL_CHECK();

  const int vs = glCreateShader(GL_VERTEX_SHADER);
  GL_CHECK();
  int len = strlen(vshader);
  glShaderSource(vs, 1, &vshader, &len);
  GL_CHECK();
  glCompileShader(vs);
  PrintCompileError(vs);

  const int fs = glCreateShader(GL_FRAGMENT_SHADER);
  GL_CHECK();
  len = strlen(fshader);
  glShaderSource(fs, 1, &fshader, &len);
  GL_CHECK();
  glCompileShader(fs);
  PrintCompileError(fs);

  prog = glCreateProgram();
  glAttachShader(prog, vs);
  GL_CHECK();
  glAttachShader(prog, fs);
  GL_CHECK();
  glLinkProgram(prog);
  GL_CHECK();

  attribute_coord2d = glGetAttribLocation(prog,  "coord2d");
  GL_CHECK();

  uniform_x_range = glGetUniformLocation(prog, "x_range");
  GL_CHECK();

  uniform_max_y = glGetUniformLocation(prog, "max_y");
  GL_CHECK();

  uniform_line_color = glGetUniformLocation(prog, "line_color");
  GL_CHECK();

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  GL_CHECK();
}

QOpenGLFramebufferObject *
GraphViewRenderer::createFramebufferObject(const QSize &size) {
  QOpenGLFramebufferObjectFormat format;
  format.setSamples(20);
  return new QOpenGLFramebufferObject(size, format);
}

GraphViewRenderer::~GraphViewRenderer() {
  while (!m_sets.empty()) {
    m_subscriber->RemoveSet(m_sets.begin()->first);
    delete m_sets.begin()->second;
    m_sets.erase(m_sets.begin());
  }
}

void
GraphViewRenderer::RenderPoints(const GraphSet::PointVec &data,
                                const float* color,
                                float max_y) {
  if (data.empty() )
    return;

  // static unsigned int last_size = 0;
  // if (last_size != data.size())
  // {
  //     std::cout << "size: " << data.size() << std::endl;
  //     for (unsigned int i = 0; i < data.size(); ++i )
  //     {
  //         std::cout << data[i].x << "," << data[i].y << std::endl;
  //     }
  // }
  // last_size = data.size();

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  GL_CHECK();

  glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(GraphSet::Point),
               data.data(), GL_STATIC_DRAW);
  GL_CHECK();

  glEnableVertexAttribArray(attribute_coord2d);
  GL_CHECK();

  glVertexAttribPointer(attribute_coord2d,   // attribute
                        2,                   // number of elements
                        // per vertex, here
                        // (x,y)
                        GL_FLOAT,            // the type of each element
                        GL_FALSE,            // take our values as-is
                        0,                   // no space between values
                        0);                  // use the vertex buffer object
  GL_CHECK();

  // GLint t = GetTimeOffset();
  // assert(t >= 0);
  // assert(t <2000);
  // float x_offset = t / 1000.0;
  GL_CHECK();

  glUniform4f(uniform_line_color, color[0], color[1], color[2], color[3]);

  glUniform1f(uniform_x_range, 60000);
  GL_CHECK();

  glUniform1f(uniform_max_y, max_y);
  GL_CHECK();

  glDrawArrays(GL_LINE_STRIP, 0, data.size());
  GL_CHECK();

  // x_offset = -2.0 + (t / 1000.0);
  // glDrawArrays(GL_LINE_STRIP, 0, t);
  // GL_CHECK();

  glDisableVertexAttribArray(attribute_coord2d);
  GL_CHECK();

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  GL_CHECK();
}

void
GraphViewRenderer::render() {
  float max = 0;
  for (std::map<int, GraphSet *>::const_iterator set = m_sets.begin();
       set != m_sets.end(); ++set) {
    const float set_max = set->second->GetMax();
    if (set_max > max)
      max = set_max;
  }

  // don't scale past 100% if the data doesn't exceed 100%
  if (max * 1.2 < 1.0 || max > 1.01)
    max *= 1.2;

  glClearColor(1, 1, 1, 1);
  GL_CHECK();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  GL_CHECK();

  glUseProgram(prog);
  GL_CHECK();

  // render the cross-lines representing 25,50,75 %
  const int power_of_ten_exponent = log10(max) - 1;
  const float power_of_ten = exp10f(power_of_ten_exponent);
  float horizontal_line_interval = 0;
  if (max / power_of_ten < 5)
    horizontal_line_interval = power_of_ten;
  else
    horizontal_line_interval = 2 * power_of_ten;

  float horizontal_line = horizontal_line_interval;
  glLineWidth(2);
  m_data.resize(2);
  m_data[0].x = 100 * 1000;
  m_data[1].x = 0;
  while (horizontal_line && horizontal_line < max) {
    m_data[0].y = horizontal_line;
    m_data[1].y = horizontal_line;
    RenderPoints(m_data, tick_lines_color, max);
    horizontal_line += horizontal_line_interval;
  }

  // render the per-10s vertical lines
  const unsigned int t = get_ms_time();
  unsigned int age_of_10_sec = t % 10000;
  // const float offset_t = 1.0  - (static_cast<float>(t % 10000))/10000.0;
  // const float line_distance = 2.0 * 10.0 / 60.0;

  // vertical line
  m_data[0].y = -1;
  m_data[1].y = 2 * max;
  m_data[0].x = age_of_10_sec;
  m_data[1].x = age_of_10_sec;

  // m_data[1].x = m_data[0].x; m_data[1].y = 1;
  while (age_of_10_sec < 60 * 1000) {
    // std::cout << m_data[0].x << ", " << m_data[0].y << " : ";
    // std::cout << m_data[1].x << ", " << m_data[1].y << std::endl;
    RenderPoints(m_data, tick_lines_color, max);
    age_of_10_sec += 10 * 1000;
    m_data[0].x = age_of_10_sec;
    m_data[1].x = age_of_10_sec;
  }

  glLineWidth(1);

  for (std::map<int, GraphSet *>::iterator set = m_sets.begin();
       set != m_sets.end(); ++set) {
    set->second->GetData(&m_data, t);
    RenderPoints(m_data, black_color, max);
  }
  update();
}

void
GraphViewRenderer::CheckError(const char * file, int line) {
  if (glGetError() == GL_NO_ERROR)
    return;
  printf("ERROR: %s:%i\n", file, line);
  exit(-1);
}

void
GraphViewRenderer::PrintCompileError(GLint shader) {
  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status == GL_TRUE)
    return;
  static const int MAXLEN = 1024;
  std::vector<char> log(MAXLEN);
  GLsizei len;
  glGetShaderInfoLog(shader,  MAXLEN,  &len, log.data());
  printf("ERROR -- compile failed: %s\n", log.data());
}

void
GraphViewRenderer::synchronize(QQuickFramebufferObject * item) {
  // call the subscriber to make a one-time request for the metrics
  // to be graphed.
  if (!m_sets.empty())
    return;

  std::vector<int> ids;
  m_subscriber->GetIDs(&ids);

  for (std::vector<int>::iterator i = ids.begin();
       i != ids.end(); ++i) {
    const int id = *i;
    GraphSet *gs = new GraphSet();
    m_sets[id] = gs;
    m_subscriber->AddSet(id, gs);
  }
}

GraphView::GraphView() : m_pub(NULL),
                         m_subscriber(NULL) {
  setTextureFollowsItemSize(true);
}


QQuickFramebufferObject::Renderer *
GraphView::createRenderer() const {
  assert(m_pub);
  assert(m_subscriber);
  QQuickFramebufferObject::Renderer * renderer =
      new GraphViewRenderer(m_subscriber, *m_pub);
  return renderer;
}

GraphView::~GraphView() {
}
