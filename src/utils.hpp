/*
 Crown Copyright 2012 AWE.

 This file is part of CloverLeaf.

 CloverLeaf is free software: you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the
 Free Software Foundation, either version 3 of the License, or (at your option)
 any later version.

 CloverLeaf is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 CloverLeaf. If not, see http://www.gnu.org/licenses/.
 */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>
#include <utility>
#include <cassert>
#include <vector>
#include <algorithm>
#include <memory>
#include <numeric>

#include "tbb/tbb.h"

namespace clover {


	struct Range1d {
		const size_t from, to;
		template<typename A, typename B>
		Range1d(A from, B to) : from(from), to(to) {
			assert(from < to);
			assert(to - from > 0);
		}
		friend std::ostream &operator<<(std::ostream &os, const Range1d &d) {
			os << "Range1d{"
			   << " X[" << d.from << "->" << d.to << " (" << (d.to - d.from) << ")]"
			   << "}";
			return os;
		}
	};

	struct Range2d {
		const size_t fromX, toX;
		const size_t fromY, toY;
		const size_t sizeX, sizeY;
		template<typename A, typename B, typename C, typename D>
		Range2d(A fromX, B fromY, C toX, D toY) :
				fromX(fromX), toX(toX), fromY(fromY), toY(toY),
				sizeX(toX - fromX), sizeY(toY - fromY) {
			assert(fromX < toX);
			assert(fromY < toY);
			assert(sizeX != 0);
			assert(sizeY != 0);
		}
		friend std::ostream &operator<<(std::ostream &os, const Range2d &d) {
			os << "Range2d{"
			   << " X[" << d.fromX << "->" << d.toX << " (" << d.sizeX << ")]"
			   << " Y[" << d.fromY << "->" << d.toY << " (" << d.sizeY << ")]"
			   << "}";
			return os;
		}
	};

#ifdef USE_VECTOR

    template<typename T>
    struct Buffer1D {

        const size_t size;
        std::vector<T> data;

        explicit Buffer1D(size_t size) : size(size), data(size) {}
        T operator[](size_t i) const { return data[i]; }
        T &operator[](size_t i) { return data[i]; }
        T *actual() { return data.data(); }

        friend std::ostream &operator<<(std::ostream &os, const Buffer1D<T> &buffer) {
            os << "Buffer1D(size: " << buffer.size << ")";
            return os;
        }

    };

    template<typename T>
    struct Buffer2D {

        const size_t sizeX, sizeY;
        std::vector<T> data;

        Buffer2D(size_t sizeX, size_t sizeY) : sizeX(sizeX), sizeY(sizeY), data(sizeX * sizeY) {}
        T &operator()(size_t i, size_t j) { return data[i + j * sizeX]; }
        T const &operator()(size_t i, size_t j) const { return data[i + j * sizeX]; }
        T *actual() { return data.data(); }

        friend std::ostream &operator<<(std::ostream &os, const Buffer2D<T> &buffer) {
            os << "Buffer2D(sizeX: " << buffer.sizeX << " sizeY: " << buffer.sizeY << ")";
            return os;
        }

    };

#else

    template<typename T>
    struct Buffer1D {

        const size_t size;
        T *data;

        explicit Buffer1D(size_t size) : size(size), data(static_cast<T *>(std::malloc(sizeof(T) * size))) {}
        Buffer1D(const Buffer1D &that) : Buffer1D(that.size) { std::copy(that.data, that.data + size, data);  }
        Buffer1D &operator=(const Buffer1D &other) = delete;
        T operator[](size_t i) const { return data[i]; }
        T &operator[](size_t i) { return data[i]; }
        T *actual() { return data; }
        virtual ~Buffer1D() { std::free(data); }

        friend std::ostream &operator<<(std::ostream &os, const Buffer1D<T> &buffer) {
            os << "Buffer1D(size: " << buffer.size << ")";
            return os;
        }

    };

    template<typename T>
    struct Buffer2D {

        const size_t sizeX, sizeY;
        T *data;

        Buffer2D(size_t sizeX, size_t sizeY) : sizeX(sizeX), sizeY(sizeY),  data(static_cast<T *>(std::malloc(sizeof(T) * sizeX * sizeY))) {}
        Buffer2D(const Buffer2D &that) : Buffer2D(that.sizeX, that.sizeY) { std::copy(that.data, that.data + sizeX * sizeY, data); }
        Buffer2D &operator=(const Buffer2D &other) = delete;
        T &operator()(size_t i, size_t j) { return data[i + j * sizeX]; }
        T const &operator()(size_t i, size_t j) const { return data[i + j * sizeX]; }
        T *actual() { return data; }
        virtual ~Buffer2D() { std::free(data); }

        friend std::ostream &operator<<(std::ostream &os, const Buffer2D<T> &buffer) {
            os << "Buffer2D(sizeX: " << buffer.sizeX << " sizeY: " << buffer.sizeY << ")";
            return os;
        }
    };

#endif

    using tbb_partitioner = tbb::auto_partitioner;
    static tbb_partitioner partitioner{};

    template<typename F>
	static constexpr void par_ranged1(const Range1d &r, const F &functor) {

        tbb::parallel_for(
                tbb::blocked_range<size_t>{r.from, r.to}, [&](const tbb::blocked_range<size_t> &r) {
                    for (size_t i = r.begin(); i < r.end(); ++i) {
                        functor(i);
                    }
                }, partitioner);
//
//		for (size_t i = r.from; i < r.to; i++) {
//			functor(i);
//		}
	}

	template<typename F>
	static constexpr void par_ranged2(const Range2d &r, const F &functor) {


        tbb::parallel_for(
                tbb::blocked_range2d<size_t>{r.fromY, r.toY, r.fromX, r.toX},
                [&](const tbb::blocked_range2d<size_t> &br) {
                    for (size_t j = br.rows().begin(); j < br.rows().end(); ++j) {
                        for (size_t i = br.cols().begin(); i < br.cols().end(); ++i) {
                            functor(i, j);
                        }
                    }

                }, partitioner);

//        for (size_t j = r.fromY; j < r.toY; j++) {
//            for (size_t i = r.fromX; i < r.toX; i++) {
//                functor(i, j);
//            }
//        }
	}


}


using namespace clover;

#endif //UTILS_HPP
