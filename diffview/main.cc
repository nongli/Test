#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>

#include <QApplication>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QScrollBar>

using namespace std;

struct DiffBlock {
  string heading;
  vector<int> a_lines;
  vector<int> b_lines;
  vector<string> lines;
};

struct Diff {
  string filename_a;
  string filename_b;
  vector<DiffBlock> regions;
};

class DiffParser {
 public:
  static bool Parse(const char* filename, vector<Diff>& diffs) {
    FILE* file = fopen(filename, "r");
    if (!file) return false;

    size_t buf_size = 1024;
    char* buf = (char*)malloc(buf_size);
    int bytes_read = 0;

    Diff* current_diff = NULL;
    DiffBlock* current_region = NULL;

    while ((bytes_read = getline(&buf, &buf_size, file)) > 0) {
      if (buf[bytes_read - 1] == '\n') {
        buf[bytes_read - 1] = '\0';
      }

      if (strncmp(buf, "diff", strlen("diff")) == 0) {
        char* a_start = strstr(buf, "a/") + 2;
        char* a_end = strchr(a_start, ' ');
        char* b_start = strstr(buf, "b/") + 2;

        diffs.resize(diffs.size() + 1);
        current_diff = &diffs[diffs.size() - 1];
        current_diff->filename_a = string(a_start, a_end - a_start);
        current_diff->filename_b = string(b_start);
        continue;
      } else if (strncmp(buf, "index", strlen("index")) == 0) {
        continue;
      } else if (strncmp(buf, "new file", strlen("new file")) == 0) {
        continue;
      } else if (strncmp(buf, "@@", strlen("@@")) == 0) {
        char* line_start = strstr(buf + 2, "@@") + 3;
        current_diff->regions.resize(current_diff->regions.size() + 1);
        current_region = &current_diff->regions[current_diff->regions.size() - 1];
        current_region->heading = string(buf, line_start - buf);
        current_region->lines.push_back(line_start);
        current_region->a_lines.push_back(current_region->lines.size() - 1);
        current_region->b_lines.push_back(current_region->lines.size() - 1);
        continue;
      } else if (strncmp(buf, "---", strlen("---")) == 0) {
        continue;
      } else if (strncmp(buf, "+++", strlen("+++")) == 0) {
        continue;
      } else if (strncmp(buf, "-", strlen("-")) == 0) {
        current_region->lines.push_back(buf + 1);
        current_region->a_lines.push_back(current_region->lines.size() - 1);
        continue;
      } else if (strncmp(buf, "+", strlen("+")) == 0) {
        current_region->lines.push_back(buf + 1);
        current_region->b_lines.push_back(current_region->lines.size() - 1);
        continue;
      } else {
        AlignRegions(current_region);
        current_region->lines.push_back(buf + 1);
        current_region->a_lines.push_back(current_region->lines.size() - 1);
        current_region->b_lines.push_back(current_region->lines.size() - 1);
        continue;
      }
    }

    free(buf);

    for (unsigned int i = 0; i < diffs.size(); ++i) {
      Diff& diff = diffs[i];
      for (unsigned int j = 0; j < diff.regions.size(); ++j) {
        DiffBlock& region = diff.regions[j];
        AlignRegions(&region);
      }
    }
    return true;
  }

  static void AlignRegions(DiffBlock* region) {
    for (unsigned int a_missing = region->a_lines.size(); 
        a_missing < region->b_lines.size(); ++a_missing) {
      region->a_lines.push_back(-1);
    }
    for (unsigned int b_missing = region->b_lines.size(); 
        b_missing < region->a_lines.size(); ++b_missing) {
      region->b_lines.push_back(-1);
    }
  }
};


class TextRenderer {
 public:
  TextRenderer(QTextCursor* a_curs, QTextCursor* b_curs) {
    a_cursor = a_curs;
    b_cursor = b_curs;

    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    font.setPointSize(9);

    header_format.setBackground(QBrush(QColor(255, 255, 100)));
    both_format.setBackground(QBrush(QColor(255, 255, 255)));
    a_format.setBackground(QBrush(QColor(255, 150, 150)));
    b_format.setBackground(QBrush(QColor(150, 255, 150)));
    
    header_format.setFont(font);
    both_format.setFont(font);
    a_format.setFont(font);
    b_format.setFont(font);
  }

  void RenderText(const Diff& diff) {
    for (unsigned int i = 0; i < diff.regions.size(); ++i) {
      const DiffBlock& region = diff.regions[i];
      a_cursor->setCharFormat(header_format);
      b_cursor->setCharFormat(header_format);
      RenderToBoth(region.heading.c_str());
      for (unsigned int j = 0; j < region.a_lines.size(); ++j) {
        int a_line = region.a_lines[j];
        int b_line = region.b_lines[j];
        if (a_line == b_line) {
          a_cursor->setCharFormat(both_format);
          b_cursor->setCharFormat(both_format);
          RenderLine(a_cursor, region.lines[a_line].c_str());
          RenderLine(b_cursor, region.lines[b_line].c_str());
        } else {
          a_cursor->setCharFormat(a_format);
          b_cursor->setCharFormat(b_format);

          if (a_line == -1) {
            RenderLine(a_cursor, "");
          } else {
            RenderLine(a_cursor, region.lines[a_line].c_str());
          }
          
          if (b_line == -1) {
            RenderLine(b_cursor, "");
          } else {
            RenderLine(b_cursor, region.lines[b_line].c_str());
          }
        }
      }

      RenderToBoth("\n");
    }
  }

  void RenderLine(QTextCursor* cursor, const char* line) {
    const int max_len = 96;
    char buf[max_len + 1];
    memset(buf, ' ', max_len);
    int len = min((int)strlen(line), max_len);
    strncpy(buf, line, len);
    buf[max_len] = '\0';
    cursor->insertText(buf);
    cursor->insertText("\n");
  }

  void RenderToBoth(const char* s) {
    a_cursor->insertText(s);
    b_cursor->insertText(s);
    a_cursor->insertText("\n");
    b_cursor->insertText("\n");
  }

  void RenderFileNames(const char* a_name, const char* b_name) {
    a_cursor->setCharFormat(header_format);
    b_cursor->setCharFormat(header_format);
    RenderLine(a_cursor, a_name);
    RenderLine(b_cursor, b_name);
  }
  
 private: 
  QTextCursor* a_cursor;
  QTextCursor* b_cursor;
  QTextCharFormat header_format;
  QTextCharFormat both_format;
  QTextCharFormat a_format;
  QTextCharFormat b_format;
};

int main(int argc, char** argv) {

  if (argc != 2) {
    printf("Usage: diffview <diff>\n");
    return 1;
  }
  const char* file = argv[1];

  vector<Diff> diffs;

  bool parsed = DiffParser::Parse(file, diffs);
  if (!parsed) return 1;

  QApplication app(argc, argv);
  QWidget window;

  QTextEdit a_editor, b_editor;
  a_editor.setReadOnly(true);
  b_editor.setReadOnly(true);
  a_editor.setMinimumWidth(700);
  b_editor.setMinimumWidth(700);
  a_editor.setMinimumHeight(700);
  b_editor.setMinimumHeight(700);

  QScrollBar* a_scroll = a_editor.verticalScrollBar();
  QScrollBar* b_scroll = b_editor.verticalScrollBar();
  a_scroll->setVisible(false);

  QObject::connect(b_scroll, SIGNAL(valueChanged(int)),
                   a_scroll, SLOT(setValue(int)));
  QObject::connect(a_scroll, SIGNAL(valueChanged(int)),
                   b_scroll, SLOT(setValue(int)));

  QTextCursor a_cursor(a_editor.textCursor());
  QTextCursor b_cursor(b_editor.textCursor());
  
  TextRenderer renderer(&a_cursor, &b_cursor);
  
  for (unsigned int i = 0; i < diffs.size(); ++i) {
    renderer.RenderFileNames(diffs[i].filename_a.c_str(), diffs[i].filename_b.c_str());
    renderer.RenderText(diffs[i]);
  }

  QHBoxLayout layout;
  layout.addWidget(&a_editor);
  layout.addWidget(&b_editor);

  window.setLayout(&layout);
  window.show();

  b_scroll->setValue(0);
  return app.exec();
}
