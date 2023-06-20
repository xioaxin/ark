// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "ark/logging.h"
#include "ark/model_io.h"

using namespace std;

namespace ark {

Tensor *Model::reduce(Tensor *input, DimType axis, Tensor *output, bool is_relu,
                      const string &name)
{
    assert(input != nullptr);
    LOG(DEBUG, "reduce ", input->shape, " ", input->ldims, " ", axis);
    OpPrecType pt;
    if (input->type == FP16) {
        pt = OP_PREC_FP16;
    } else if (input->type == FP32) {
        pt = OP_PREC_FP32;
    } else {
        LOGERR("unsupported input data type: ", type_str(input->type));
    }
    if (output != nullptr && input->type != output->type) {
        LOGERR("invalid output data type: ", type_str(output->type));
    }
    if (output == nullptr) {
        Dims reduced_shape{input->shape};
        reduced_shape[axis] = 1;
        output = this->tensor(reduced_shape, input->type);
    } else if (output == input) {
        LOGERR(
            "output tensor cannot be the same as input tensor for reduce op");
    }
    this->create_op(OP_REDUCE, pt, {input}, {output}, {is_relu, axis}, name);
    return output;
}

} // namespace ark
