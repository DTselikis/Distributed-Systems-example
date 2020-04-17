struct intMatrix {
  int numArray<100>;
};

struct floatMatrix {
  float muledArray<100>;
};

struct mulFloat {
  int numArray<100>;
  float multiplier;
};

program RPC_PROG {
  version RPC_VERS {
    float findAverage(intMatrix) = 1;
    intMatrix findMinMax(intMatrix) = 2;
    floatMatrix mulMatrixWithFloat(mulFloat) = 3;
  } = 1;
} = 0x23451111;
