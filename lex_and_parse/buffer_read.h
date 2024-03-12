#pragma once
#include <cstdio>
#include <cassert>
#include <string>

const unsigned buffer_len = 1000;

// scanner
class BufferRead {
 public:
    BufferRead(FILE* file_in, std::string filename_in):file(file_in), filename_(filename_in){   
        buffer_len_ = fread(buffer, 1, buffer_len, file);
    }
    ~BufferRead() {
        fclose(file);
    }
    char GetChar() {
        assert(file);
        if (cur_pos_ == buffer_len_) {
            if (buffer_len_ != buffer_len) {
                return -1;
            } else {
                buffer_len_ = fread(buffer, 1, buffer_len, file);
                cur_pos_ = 0;
            }
        }
        char ret = buffer[cur_pos_];
        if (ret == '\n') {
            ++line_num_;
            col_num_ = 0;
        } else {
            ++col_num_;
        }
        return ret;
    }
    char AdvanceAndGetChar() {
        cur_pos_++;  // iterate
        return GetChar();
    }
    unsigned GetLineNum() const {
        return line_num_;
    }
    unsigned GetColNum() const {
        return col_num_;
    }
 private:
    unsigned line_num_ = 0, col_num_ = 0, buffer_len_ = 0;
    int cur_pos_ = 0;
    FILE* file;
    std::string filename_;
    char buffer[buffer_len];
};