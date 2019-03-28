#include "json_writer.h"
#include <sstream>

std::string JsonWriter::EscapeJson(const std::string &s) {
    std::ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        switch (*c) {
        case '\\': o << "\\\\"; break;
        case '\"': o << "\\\""; break;
        case '\r': o << "\\r" ; break;
        case '\n': o << "\\n" ; break;
        default: o << *c;
        }
    }
    return o.str();
}

void JsonWriter::Tabulate() {
  // Two space tabs_
  int spaces = 2 * tabs_;
  for(int i = 0; i < spaces; i++) {
    fputc(' ', stream_);
  }
}


void JsonWriter::StartObject(bool continued) {
  if(continued) {
    fprintf(stream_, ",\n");
  }
  Tabulate();
  fprintf(stream_, "{\n");
  tabs_++;
}


void JsonWriter::StringProperty(std::string property, std::string value, bool continued) {
  if(continued) {
    fprintf(stream_, ",\n");
  }
  Tabulate();
  fprintf(stream_, "\"%s\": \"%s\"", property.c_str(), EscapeJson(value).c_str());
}

void JsonWriter::BoolProperty(std::string property, bool value, bool continued) {
  if(continued) {
    fprintf(stream_, ",\n");
  }
  Tabulate();
  fprintf(stream_, "\"%s\": %s", property.c_str(), value ? "true" : "false");
}

void JsonWriter::String(std::string s, bool continued) {
  if(continued) {
    fprintf(stream_, ",\n");
  }
  Tabulate();
  fprintf(stream_, "\"%s\"", EscapeJson(s).c_str());
}

void JsonWriter::NumericalStringProperty(std::string property, int value, bool continued) {
  if(continued) {
    fprintf(stream_, ",\n");
  }
  Tabulate();
  fprintf(stream_, "\"%s\": \"%d\"", property.c_str(), value);
}

void JsonWriter::StartArrayProperty(std::string name, bool continued) {
  if(continued) {
    fprintf(stream_, ",\n");
  }
  Tabulate();
  fprintf(stream_, "\"%s\": [\n", name.c_str());
  tabs_++;
}

void JsonWriter::EndArray() {
  if(tabs_ > 0) 
    tabs_--;

  fputc('\n', stream_);
  Tabulate();
  fputc(']', stream_);
  
}

void JsonWriter::StartObjectProperty(std::string name, bool continued) {
  if(continued) {
    fprintf(stream_, ",\n");
  }

  Tabulate();
  fprintf(stream_, "\"%s\": {\n", name.c_str());
  tabs_++;
}


 void JsonWriter::EndObject() {
  if(tabs_ > 0) 
    tabs_--;

  fputc('\n', stream_);
  Tabulate();
  fputc('}', stream_);

}
