README for fast_vector_append: doing std::vector::push_back without copying

fast_vector_append.h is a header-only C++ library (both C++98 and C++11 and
newer) for making std::vector::push_back faster by making it do fewer copies
on the to-be-appended item. It autodetects the situation, and calls
v.push_back(...), v.emplace_back(...) or v.last().swap(...), whichever is
fastest and available.

See the file fast_vector_append.h for more documentation about usage.

See the file test_fast_vector_append.cc with actual usage with user-defined
classes.

__END__
