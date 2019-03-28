#ifndef NINJA_JSON_WRITER_H_
#define NINJA_JSON_WRITER_H_

#include <stdio.h>
#include <string>

/// Writes JSON into a stream (stdout by default). 
/// The idea is to build the elements step by step, as if you were typing them by hand.
/// The writer keeps track of indentation but doesn't ensure a correct JSON if its methods aren't called properly.
/// An example:
///     {
///       foo: {
///         bar: "5",
///         baz: "zzz"
///         }
///     }
////
///  would be produced by calling
///  StartObject();
///  StartObjectProperty("foo", false);
///  NumericalStringProperty("bar", 5, false)
///  StringProperty("baz", "zzz")
///  EndObject();
///  EndObject();
struct JsonWriter {

    JsonWriter(FILE* stream = stdout) {
        stream_ = stream;
        tabs_ = 0;
    };

    /// Escape a string to a JSON string (e.g, backslashes)
    static std::string EscapeJson(const std::string &s);

    /// In the following functions, a true 'continued' param means the thing to write is part of a list (object propety or array) 
    ///   and it's not the first of the list.
    void StartObject(bool continued);
    void StartArrayProperty(std::string name, bool continued);
    void EndArray();
    void StringProperty(std::string property, std::string value, bool continued);
    void JsonWriter::BoolProperty(std::string property, bool value, bool continued);

    void String(std::string s, bool continued);

    /// Write a property which has a number as value, but inside a string. 
    /// (e.g, for { foo: "4" } one would call NumericalStringProperty("foo", 4, false))
    void NumericalStringProperty(std::string property, int value, bool continued);
    
    /// Start an object which is a property of another object
    void StartObjectProperty(std::string name, bool continued);
    void EndObject();

    private:
        void Tabulate();
        int tabs_;
        FILE* stream_;
};

#endif