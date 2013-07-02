package io.soundbyte.core;

/**
 * Soundbyte engine configuration parameters.
 * 
 * Don't mess with these. See {@link NativeEngine#defaultConfiguration()}.
 */
public class EngineConfiguration {
  /** The base (lowest) subcarrier frequency. */
  public final int baseFrequency;
  /** The number of parallel subcarriers. */
  public final int subcarriers;
  /** The spacing (in subcarrier-widths) of adjacent subcarriers. */
  public final int subcarrierSpacing;
  /** The rate of chips, in Hz. */
  public final int chipRate;
  
  public EngineConfiguration(int baseFrequency, int subcarriers, int subcarrierSpacing,
      int chipRate) {
    this.baseFrequency = baseFrequency;
    this.subcarriers = subcarriers;
    this.subcarrierSpacing = subcarrierSpacing;
    this.chipRate = chipRate;
  }
}
