/*
 * iomanip.h
 *
 *  Created on: Aug 9, 2013
 *      Author: vincent
 */

#ifndef ATLAS_IOMANIP_H_
#define ATLAS_IOMANIP_H_

/*
 * data_time.h
 *
 *  Created on: Apr 8, 2011
 *      Author: Vincent Zhang, ivincent.zhang@gmail.com
 */

/*    Copyright 2011 ~ 2013 Vincent Zhang, ivincent.zhang@gmail.com
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <cstdio>
#include <cstring>
#include <chrono>
#include <iomanip>

#ifndef STD_PUT_TIME

namespace atlas {

  // miss std::put_time, use this function instead
  inline std::string put_time(std::tm* tm, const char* fmt) {
    char buffer[128];
    std::memset(buffer, 0, sizeof(buffer));
    std::strftime(buffer, sizeof(buffer), fmt, tm);

    std::string tmp(buffer);
    return tmp;
  }
}

#endif // not define STD_PUT_TIME

#endif /* IOMANIP_H_ */
