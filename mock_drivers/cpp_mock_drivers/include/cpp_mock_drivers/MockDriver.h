/*
 * Copyright (C) 2019-2020 LEIDOS.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
#pragma once

#include <ros/ros.h>
#include <string>
#include <vector>

#include "cpp_mock_drivers/ROSComms.h"
#include "cpp_mock_drivers/MockDriverNode.h"
#include "cpp_mock_drivers/comm_types.h"

namespace mock_drivers{

    class MockDriver{

        protected:

        MockDriverNode mock_driver_node_;


        public:

        virtual int run() = 0;

    };
    
}