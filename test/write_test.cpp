/*
 * libnbt++ - A library for the Minecraft Named Binary Tag format.
 * Copyright (C) 2013, 2015  ljfa-ag
 *
 * This file is part of libnbt++.
 *
 * libnbt++ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libnbt++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libnbt++.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "microtest.h"
#include "io/stream_writer.h"
#include "nbt_tags.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace nbt;

void test_stream_writer_big()
{
    std::ostringstream os;
    nbt::io::stream_writer writer(os);

    ASSERT(&writer.get_ostr() == &os);
    ASSERT(writer.get_endian() == endian::big);

    writer.write_type(tag_type::End);
    writer.write_type(tag_type::Long);
    writer.write_type(tag_type::Int_Array);

    writer.write_num(int64_t(0x0102030405060708));

    writer.write_string("foobar");

    ASSERT(os);
    std::string expected{
        0, //tag_type::End
        4, //tag_type::Long
        11, //tag_type::Int_Array

        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, //0x0102030405060708 in Big Endian

        0x00, 0x06, //string length in Big Endian
        'f', 'o', 'o', 'b', 'a', 'r'
    };
    std::string s = os.str();
    ASSERT(os.str() == expected);
    std::clog << "test_stream_writer_big passed" << std::endl;
}

void test_stream_writer_little()
{
    std::ostringstream os;
    nbt::io::stream_writer writer(os, endian::little);

    ASSERT(writer.get_endian() == endian::little);

    writer.write_num(int32_t(0x0a0b0c0d));

    writer.write_string("foobar");

    ASSERT(os);
    std::string expected{
        0x0d, 0x0c, 0x0b, 0x0a, //0x0a0b0c0d in Little Endian

        0x06, 0x00, //string length in Little Endian
        'f', 'o', 'o', 'b', 'a', 'r'
    };
    std::string s = os.str();
    ASSERT(os.str() == expected);
    std::clog << "test_stream_writer_little passed" << std::endl;
}

void test_write_tags_big()
{
    std::ostringstream os;
    nbt::io::stream_writer writer(os);

    //tag_primitive
    writer.write_payload(tag_byte(127));
    writer.write_payload(tag_short(32767));
    writer.write_payload(tag_int(2147483647));
    writer.write_payload(tag_long(9223372036854775807));

    //Same values as in endian_str_test
    writer.write_payload(tag_float(std::stof("-0xCDEF01p-63")));
    writer.write_payload(tag_double(std::stod("-0x1DEF0102030405p-375")));

    ASSERT((os.str() == std::string{
        '\x7F',
        '\x7F', '\xFF',
        '\x7F', '\xFF', '\xFF', '\xFF',
        '\x7F', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF',

        '\xAB', '\xCD', '\xEF', '\x01',
        '\xAB', '\xCD', '\xEF', '\x01', '\x02', '\x03', '\x04', '\x05'
    }));
    os.str(""); //clear and reuse the stream

    //tag_string
    writer.write_payload(tag_string("barbaz"));
    ASSERT((os.str() == std::string{
        0x00, 0x06, //string length in Big Endian
        'b', 'a', 'r', 'b', 'a', 'z'
    }));

    //tag_byte_array
    os.str("");
    writer.write_payload(tag_byte_array{0, 1, 127, -128, -127});
    ASSERT((os.str() == std::string{
        0x00, 0x00, 0x00, 0x05, //length in Big Endian
        0, 1, 127, -128, -127
    }));
    os.str("");

    //tag_int_array
    writer.write_payload(tag_int_array{0x01020304, 0x05060708, 0x090a0b0c});
    ASSERT((os.str() == std::string{
        0x00, 0x00, 0x00, 0x03, //length in Big Endian
        0x01, 0x02, 0x03, 0x04,
        0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c
    }));
    os.str("");

    //tag_list
    writer.write_payload(tag_list()); //empty list with undetermined type, should be written as list of tag_end
    writer.write_payload(tag_list(tag_type::Int)); //empty list of tag_int
    writer.write_payload(tag_list{ //nested list
        tag_list::of<tag_short>({0x3456, 0x789a}),
        tag_list::of<tag_byte>({0x0a, 0x0b, 0x0c, 0x0d})
    });
    ASSERT((os.str() == std::string{
        0, //tag_type::End
        0x00, 0x00, 0x00, 0x00, //length

        3, //tag_type::Int
        0x00, 0x00, 0x00, 0x00, //length

        9, //tag_type::List
        0x00, 0x00, 0x00, 0x02, //length
        //list 0
        2, //tag_type::Short
        0x00, 0x00, 0x00, 0x02, //length
        '\x34', '\x56',
        '\x78', '\x9a',
        //list 1
        1, //tag_type::Byte
        0x00, 0x00, 0x00, 0x04, //length
        0x0a,
        0x0b,
        0x0c,
        0x0d
    }));
    os.str("");

    //tag_compound
    //Testing if writing compounds works properly is problematic because the
    //order of the tags is not guaranteed. However with only two tags in a
    //compound we only have two possible orderings.
    writer.write_payload(tag_compound{});
    writer.write_payload(tag_compound{
        {"foo", "quux"},
        {"bar", tag_int(0x789abcde)}
    });

    std::string endtag{0x00};
    std::string subtag1{
        8, //tag_type::String
        0x00, 0x03, //key length
        'f', 'o', 'o',
        0x00, 0x04, //string length
        'q', 'u', 'u', 'x'
    };
    std::string subtag2{
        3, //tag_type::Int
        0x00, 0x03, //key length
        'b', 'a', 'r',
        '\x78', '\x9A', '\xBC', '\xDE'
    };
    ASSERT(os.str() == endtag + subtag1 + subtag2 + endtag
        || os.str() == endtag + subtag2 + subtag1 + endtag);
}

int main()
{
    test_stream_writer_big();
    test_stream_writer_little();
    test_write_tags_big();
}
