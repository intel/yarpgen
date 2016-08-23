/*
Copyright (c) 2015-2016, Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <crosschain/lbbuilder.h>

using namespace crosschain;


int main() {
    //rl::rand_val_gen = std::make_shared<rl::RandValGen>(2289942116);

    rl::GenPolicy policy;
    policy.set_min_array_size(100);
    policy.set_max_array_size(1000);
    std::shared_ptr<rl::Context> ctx = std::make_shared<rl::Context>(policy, 
                                                                     std::shared_ptr<rl::Stmt>(), 
                                                                     std::shared_ptr<rl::Context>());

    ctx->set_verbose_level(0);
    ctx->echo_seed();
    std::vector<std::shared_ptr<Vector>> input;

    std::vector<ull> dim;
    dim.push_back(40);
    dim.push_back(8);
    dim.push_back(15);


    std::shared_ptr<VectorDeclStmt> V_in = std::make_shared<VectorDeclStmt>(ctx, "v_in", VecElem::Kind::C_ARR, rl::IntegerType::IntegerTypeID::INT, 800);
    input.push_back(V_in->get_data());


    std::shared_ptr<VectorDeclStmt> V_out = std::make_shared<VectorDeclStmt>(ctx, "v_out", VecElem::Kind::C_ARR, rl::IntegerType::IntegerTypeID::INT, 200);
    V_out->setPurpose(VecElem::Purpose::WONLY);
    input.push_back(V_out->get_data());



    std::shared_ptr<LBBuilder> lbb = std::make_shared<LBBuilder>(ctx, input);

    // Initialization workarond for performance testing

    std::cout << "#include <climits>\n";
    std::cout << "#include <iostream>\n";
    std::cout << "#include <array>\n";
    std::cout << "#include <vector>\n";
    std::cout << "#include <valarray>\n";
    std::cout << "int main(){\n";
    std::cout << V_in->emit() << "\n";
    std::cout << V_out->emit() << "\n";
    std::cout << V_in->getInitStmt()->emit();
    std::cout << V_out->getInitStmt()->emit();

    std::cout << lbb->emit();

    std::cout << "}\n";

    return 0;
}
