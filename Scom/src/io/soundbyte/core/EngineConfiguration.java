package io.soundbyte.core;

public class EngineConfiguration {
  public final int baseFrequency;
  public final int subcarriers;
  public final int subcarrierSpacing;
  public final int chipRate;
  
  public EngineConfiguration(int baseFrequency, int subcarriers, int subcarrierSpacing,
      int chipRate) {
    this.baseFrequency = baseFrequency;
    this.subcarriers = subcarriers;
    this.subcarrierSpacing = subcarrierSpacing;
    this.chipRate = chipRate;
  }
}
