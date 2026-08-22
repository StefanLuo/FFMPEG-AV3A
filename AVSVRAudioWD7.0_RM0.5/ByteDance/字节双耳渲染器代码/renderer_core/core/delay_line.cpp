/* Copyright 2021 Beijing Zitiao Network Technology Co.,
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "delay_line.h"

namespace avs3renderer {

template <>
float DelayLine<float, float>::GetDefaultElementValue() {
    return 0.f;
}

template <>
double DelayLine<double, float>::GetDefaultElementValue() {
    return 0.0;
}

template <>
float DelayLine<float, double>::GetDefaultElementValue() {
    return 0.f;
}

template <>
double DelayLine<double, double>::GetDefaultElementValue() {
    return 0.0;
}

}  // namespace avs3renderer
