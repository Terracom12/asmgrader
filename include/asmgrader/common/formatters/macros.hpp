#pragma once

#include <asmgrader/common/formatters/aggregate.hpp>
#include <asmgrader/common/formatters/enum.hpp>
#include <asmgrader/common/static_string.hpp>

#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/view/transform.hpp>

#define FMT_SERIALIZE_CLASS_MEMBER_IMPL(r, class_name, i, ident)                                                       \
    BOOST_PP_COMMA_IF(i) std::pair {                                                                                   \
        ::asmgrader::StaticString{BOOST_PP_STRINGIZE(ident)}, &class_name::ident                                       \
    }

#define FMT_SERIALIZE_CLASS(class_name, ... /*members*/)                                                               \
    template <>                                                                                                        \
    struct fmt::formatter<class_name>                                                                                  \
        : ::asmgrader::detail::AggregateFormatter<class_name, #class_name                                             \
                                                  __VA_OPT__(, BOOST_PP_SEQ_FOR_EACH_I(                                  \
                                                      FMT_SERIALIZE_CLASS_MEMBER_IMPL, class_name,                     \
                                                      BOOST_PP_TUPLE_TO_SEQ(BOOST_PP_TUPLE_SIZE((__VA_ARGS__)),        \
                                                                            (__VA_ARGS__))))>                          \
    {                                                                                                                  \
    }

#define FMT_SERIALIZE_ENUMERATOR_IMPL(r, enum_name, i, ident)                                                          \
    BOOST_PP_COMMA_IF(i) std::pair {                                                                                   \
        ::asmgrader::StaticString{BOOST_PP_STRINGIZE(ident)}, enum_name::ident                                         \
    }

#define FMT_SERIALIZE_ENUM(enum_name, ... /*enumerators*/)                                                             \
    template <>                                                                                                        \
    struct fmt::formatter<enum_name>                                                                                   \
        : ::asmgrader::detail::EnumFormatter<enum_name, #enum_name                                                    \
                                             __VA_OPT__(, BOOST_PP_SEQ_FOR_EACH_I(                                       \
                                                 FMT_SERIALIZE_ENUMERATOR_IMPL, enum_name,                             \
                                                 BOOST_PP_TUPLE_TO_SEQ(BOOST_PP_TUPLE_SIZE((__VA_ARGS__)),             \
                                                                       (__VA_ARGS__))))>                               \
    {                                                                                                                  \
    }
