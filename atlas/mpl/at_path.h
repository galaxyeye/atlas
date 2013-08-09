/*
 * type_tree.h
 *
 *  Created on: Apr 14, 2013
 *      Author: vincent
 */

#ifndef ATLAS_MPL_AT_PATH_H_
#define ATLAS_MPL_AT_PATH_H_

namespace atlas {
  namespace mpl {

    namespace {

      template<size_t I, typename TypeTree, typename... Path>
      struct __at_path;

      template<size_t I, typename TypeTree, typename N, typename N2>
      struct __at_path<I, TypeTree, N, N2> {
        using boost::mpl::at;

        typedef typename at<typename at<TypeTree, N>::type, N2>::type type;
      };

      template<size_t I, typename TypeTree, typename Header, typename... Tail>
      struct __at_path<I, TypeTree, Header, Tail...> {
        typedef typename __at_path<I + 1, TypeTree, Tail...>::type type;
      };

    } // anonymous

    template<typename TypeTree, typename... Path>
    struct at_path : public __at_path<0, TypeTree, Path...> {};

  } // mpl
} // atlas

#endif /* ATLAS_MPL_AT_PATH_H_ */
