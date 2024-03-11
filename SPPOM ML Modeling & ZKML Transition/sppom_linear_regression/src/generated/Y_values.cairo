use array::ArrayTrait;
use orion::operators::tensor::{FP16x16Tensor, TensorTrait, Tensor};
use orion::numbers::{FixedTrait, FP16x16, FP16x16Impl};

fn Y_values() -> Tensor<FP16x16>  {
    let mut shape = ArrayTrait::new();
    shape.append(25);
    let mut data = ArrayTrait::new();
    data.append(FixedTrait::new(5748073, false ));
    data.append(FixedTrait::new(6935864, false ));
    data.append(FixedTrait::new(7470835, false ));
    data.append(FixedTrait::new(7371522, false ));
    data.append(FixedTrait::new(6745209, false ));
    data.append(FixedTrait::new(5928598, false ));
    data.append(FixedTrait::new(6980356, false ));
    data.append(FixedTrait::new(7532800, false ));
    data.append(FixedTrait::new(7328629, false ));
    data.append(FixedTrait::new(6506004, false ));
    data.append(FixedTrait::new(5684360, false ));
    data.append(FixedTrait::new(6851909, false ));
    data.append(FixedTrait::new(7489422, false ));
    data.append(FixedTrait::new(7350995, false ));
    data.append(FixedTrait::new(6521991, false ));
    data.append(FixedTrait::new(5176711, false ));
    data.append(FixedTrait::new(6539455, false ));
    data.append(FixedTrait::new(7357539, false ));
    data.append(FixedTrait::new(7383628, false ));
    data.append(FixedTrait::new(6810487, false ));
    data.append(FixedTrait::new(4570071, false ));
    data.append(FixedTrait::new(6022759, false ));
    data.append(FixedTrait::new(7086570, false ));
    data.append(FixedTrait::new(7374574, false ));
    data.append(FixedTrait::new(7136390, false ));
let tensor = TensorTrait::<FP16x16>::new(shape.span(), data.span()); 
 
return tensor;

}
