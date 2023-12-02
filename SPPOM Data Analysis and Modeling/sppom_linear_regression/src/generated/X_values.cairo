use array::ArrayTrait;
use orion::operators::tensor::{FP16x16Tensor, TensorTrait, Tensor};
use orion::numbers::{FixedTrait, FP16x16, FP16x16Impl};

fn X_values() -> Tensor<FP16x16>  {
    let mut shape = ArrayTrait::new();
    shape.append(25);
    shape.append(1);
    let mut data = ArrayTrait::new();
    data.append(FixedTrait::new(1201918, false ));
    data.append(FixedTrait::new(1650839, false ));
    data.append(FixedTrait::new(1802898, false ));
    data.append(FixedTrait::new(1739335, false ));
    data.append(FixedTrait::new(1500195, false ));
    data.append(FixedTrait::new(1272136, false ));
    data.append(FixedTrait::new(1663187, false ));
    data.append(FixedTrait::new(1830792, false ));
    data.append(FixedTrait::new(1718906, false ));
    data.append(FixedTrait::new(1436703, false ));
    data.append(FixedTrait::new(1188762, false ));
    data.append(FixedTrait::new(1620149, false ));
    data.append(FixedTrait::new(1804606, false ));
    data.append(FixedTrait::new(1728389, false ));
    data.append(FixedTrait::new(1442674, false ));
    data.append(FixedTrait::new(975434, false ));
    data.append(FixedTrait::new(1522253, false ));
    data.append(FixedTrait::new(1773639, false ));
    data.append(FixedTrait::new(1770396, false ));
    data.append(FixedTrait::new(1555910, false ));
    data.append(FixedTrait::new(703187, false ));
    data.append(FixedTrait::new(1290003, false ));
    data.append(FixedTrait::new(1673246, false ));
    data.append(FixedTrait::new(1798933, false ));
    data.append(FixedTrait::new(1715526, false ));
let tensor = TensorTrait::<FP16x16>::new(shape.span(), data.span()); 
 
return tensor;

}
