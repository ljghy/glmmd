#ifndef GLMMD_PARALLEL_FOR_EACH_H_
#define GLMMD_PARALLEL_FOR_EACH_H_

#ifndef GLMMD_DONT_PARALLELIZE
#ifdef GLMMD_USE_TBB
#include <tbb/parallel_for_each.h>
#else
#include <algorithm>
#include <execution>
#endif
#endif

namespace glmmd
{

template <typename Iter, typename Func>
void parallelForEach(Iter first, Iter last, Func func)
{

#ifndef GLMMD_DONT_PARALLELIZE
#ifdef GLMMD_USE_TBB
    tbb::parallel_for_each(first, last, func);
#else
    std::for_each(std::execution::par, first, last, func);
#endif
#else
    std::for_each(first, last, func);
#endif
}

} // namespace glmmd

#endif
