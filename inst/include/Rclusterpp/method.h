#ifndef RCLUSTERP_METHOD_H
#define RCLUSTERP_METHOD_H

#include <iostream>

namespace Rclusterpp {

	
	template<class Cluster>
	struct DistanceFunctor {
		typedef Cluster                         first_argument_type;
		typedef Cluster                         second_argument_type;
		typedef typename Cluster::distance_type result_type;
	};

	template<class Cluster>
	struct MergeFunctor {
		typedef Cluster                         first_argument_type;
		typedef Cluster                         second_argument_type;
		typedef Cluster                         third_argument_type;
		typedef void														result_type;
	};


	namespace Methods {

		// Distance
		// ----------------------------------------

		template<class V>
		double euclidean_distance(const V& a, const V& b) {
			using namespace arma;
			return (double)sqrt( accu( square( a - b ) ) );
		}

		// Distance Adaptors
		
		template<class Matrix, class Distance>
		class DistanceFromStoredDataRows : public std::binary_function<size_t, size_t, typename Distance::result_type> {
			public:
				typedef typename DistanceFromStoredDataRows::result_type result_type;

				DistanceFromStoredDataRows(const Matrix& data, Distance distance) : data_(data), distance_(distance) {}	

				result_type operator()(size_t i1, size_t i2) const {
					return distance_(data_.row(i1), data_.row(i2));
				}

			private:	
				DistanceFromStoredDataRows() {}

				const Matrix& data_;
				Distance distance_;
		};

		// Link Adaptors
		
		template<class Cluster, class Distance>
		class AverageLink : public DistanceFunctor<Cluster> {
			public:
				typedef typename AverageLink::result_type result_type;
			
				AverageLink(Distance d) : d_(d) {}
			
				result_type operator()(const Cluster& c1, const Cluster& c2) const {
					result_type result = 0.;
					typedef typename Cluster::idx_const_iterator iter;
					for (iter i=c1.idxs_begin(), ie=c1.idxs_end(); i!=ie; ++i) {
						for (iter j=c2.idxs_begin(), je=c2.idxs_end(); j!=je; ++j) {
							result += d_(*i, *j);
						}
					}
					return result / (c1.size() * c2.size());
				}

			private:
				Distance d_;
		};

		template<class Cluster>
		struct WardsLink : DistanceFunctor<Cluster> {
			typedef typename Cluster::distance_type result_type;		
			result_type operator()(const Cluster& c1, const Cluster& c2) const {
				return arma::accu( square( c1.center() - c2.center() ) ) * (c1.size() * c2.size()) / (c1.size() + c2.size()); 
			}
		};

		// Merge 
		// ----------------------------------------

		template<class Cluster>
		struct NoOpMerge : public MergeFunctor<Cluster> {
			void operator()(Cluster& co, const Cluster& c1, const Cluster& c2) const {
				// Noop
				return;
			}
		};

		template<class Cluster>
		struct WardsMerge : MergeFunctor<Cluster> {									
			void operator()(Cluster& co, const Cluster& c1, const Cluster& c2) const {
				co.set_center( ((c1.center() * c1.size()) + (c2.center() * c2.size())) / co.size() );
			}
		};

	} // end of Methods namespace


	template<class Cluster, class Distancer, class Merger> 
	struct LinkageMethod {
		typedef Cluster   cluster_type;
		
		typedef Distancer distancer_type;
		typedef Merger    merger_type;

		typedef typename Distancer::result_type distance_type;

		Distancer distancer;
		Merger    merger;

		LinkageMethod(Distancer distancer=Distancer(), Merger merger=Merger()) : 
			distancer(distancer), merger(merger) {}
	};


	template<class Matrix, class Distance>
	Methods::DistanceFromStoredDataRows<Matrix, Distance> stored_data_rows(const Matrix& m, Distance d) {
		return Methods::DistanceFromStoredDataRows<Matrix, Distance>(m, d);
	}

	template<class Matrix, class Result, class Vector>
	Methods::DistanceFromStoredDataRows<Matrix, std::pointer_to_binary_function<Vector, Vector, Result> > 
	stored_data_rows(const Matrix& m, Result (*d)(Vector, Vector)) {
		return stored_data_rows(m, std::ptr_fun(d)); 
	}

	template<class Matrix>
	Methods::DistanceFromStoredDataRows<
		Matrix, 
		std::pointer_to_binary_function<const arma::Row<typename Matrix::elem_type>&, const arma::Row<typename Matrix::elem_type>&, double> 
	> 
	stored_data_rows(const Matrix& m, DistanceKinds dk) {
		switch (dk) {
			default:
				throw std::invalid_argument("Linkage or distance method not yet supported");
			case Rclusterpp::EUCLIDEAN:
				return stored_data_rows(m, &Methods::euclidean_distance<arma::Row<typename Matrix::elem_type> >);
		}
	}

	template<class Cluster>
	LinkageMethod<Cluster, Methods::WardsLink<Cluster>, Methods::WardsMerge<Cluster> > wards_linkage() {
		return LinkageMethod<Cluster, Methods::WardsLink<Cluster>, Methods::WardsMerge<Cluster> >();
	}

	template<class Cluster, class Distance>
	LinkageMethod<Cluster, Methods::AverageLink<Cluster, Distance>, Methods::NoOpMerge<Cluster> > average_linkage(Distance d) {
		return LinkageMethod<Cluster, Methods::AverageLink<Cluster, Distance>, Methods::NoOpMerge<Cluster> >(
				Methods::AverageLink<Cluster, Distance>(d)
				); 
	}
	
	
} // end of Rclusterpp namespace

#endif
