/* 70 trivial functions in one TU — exceeds the old MAX_FUNCS=64 limit.
 * Each returns its index; main sums a few and returns 42. */
int f01() { return 1; }
int f02() { return 2; }
int f03() { return 3; }
int f04() { return 4; }
int f05() { return 5; }
int f06() { return 6; }
int f07() { return 7; }
int f08() { return 8; }
int f09() { return 9; }
int f10() { return 10; }
int f11() { return 11; }
int f12() { return 12; }
int f13() { return 13; }
int f14() { return 14; }
int f15() { return 15; }
int f16() { return 16; }
int f17() { return 17; }
int f18() { return 18; }
int f19() { return 19; }
int f20() { return 20; }
int f21() { return 21; }
int f22() { return 22; }
int f23() { return 23; }
int f24() { return 24; }
int f25() { return 25; }
int f26() { return 26; }
int f27() { return 27; }
int f28() { return 28; }
int f29() { return 29; }
int f30() { return 30; }
int f31() { return 31; }
int f32() { return 32; }
int f33() { return 33; }
int f34() { return 34; }
int f35() { return 35; }
int f36() { return 36; }
int f37() { return 37; }
int f38() { return 38; }
int f39() { return 39; }
int f40() { return 40; }
int f41() { return 41; }
int f42() { return 42; }
int f43() { return 43; }
int f44() { return 44; }
int f45() { return 45; }
int f46() { return 46; }
int f47() { return 47; }
int f48() { return 48; }
int f49() { return 49; }
int f50() { return 50; }
int f51() { return 51; }
int f52() { return 52; }
int f53() { return 53; }
int f54() { return 54; }
int f55() { return 55; }
int f56() { return 56; }
int f57() { return 57; }
int f58() { return 58; }
int f59() { return 59; }
int f60() { return 60; }
int f61() { return 61; }
int f62() { return 62; }
int f63() { return 63; }
int f64() { return 64; }
int f65() { return 65; }
int f66() { return 66; }
int f67() { return 67; }
int f68() { return 68; }
int f69() { return 69; }
int f70() { return 70; }

int main() {
    /* call across the old 64-function boundary */
    return f01() + f64() + f70() - f01() - f33() - f59();
}
