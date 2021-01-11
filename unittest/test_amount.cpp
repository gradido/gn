#include "test_common.h"
#include "blockchain_gradido_def.h"

TEST(GradidoAmount, smoke) {
    GradidoValue val;
    val.amount = 17;
    val.decimal_amount = 21;

    GradidoValue val2;
    val2.amount = 3;
    val2.decimal_amount = 7;
    
    GradidoValue val3 = val + val2;
    ASSERT_EQ(val3.amount, val.amount + val2.amount);
    ASSERT_EQ(val3.decimal_amount, val.decimal_amount + 
              val2.decimal_amount);
    ASSERT_EQ(val < val3, true);
    ASSERT_EQ(val2 < val3, true);
    ASSERT_EQ(val3 < val, false);
    ASSERT_EQ(val3 < val2, false);
    
    GradidoValue val4 = val;
    ASSERT_EQ(val < val4, false);
    ASSERT_EQ(val4 < val, false);

    GradidoValue val5 = val3 - val2;
    ASSERT_EQ(val5.amount, val.amount);
    ASSERT_EQ(val5.decimal_amount, val.decimal_amount);

    /////

    GradidoValue val6;
    val6.amount = 5;
    val6.decimal_amount = GRADIDO_VALUE_DECIMAL_AMOUNT_BASE - 1;

    GradidoValue val7;
    val7.amount = 0;
    val7.decimal_amount = 1;

    GradidoValue val8 = val6 + val7;
    ASSERT_EQ(val8.amount, 6);
    ASSERT_EQ(val8.decimal_amount, 0);

    GradidoValue val9;
    val9 = val8 - val7;
    ASSERT_EQ(val9.amount, val6.amount);
    ASSERT_EQ(val9.decimal_amount, val6.decimal_amount);

    ///
    
    try {
        val6 - val5;
        ASSERT_EQ(true, false);
    } catch (...) {
        // all good
    }
    
    ////////
    GradidoValue val10;
    val10 += val;
    
    ASSERT_EQ(val10.amount, val.amount);
    ASSERT_EQ(val10.decimal_amount, val.decimal_amount);


    
}
