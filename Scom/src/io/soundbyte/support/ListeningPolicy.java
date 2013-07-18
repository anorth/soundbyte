package io.soundbyte.support;

/**
 * A policy that indicates when an {@link AudioListener} can listen.
 */
public interface ListeningPolicy {
  /** Whether a listener should listen now. */
  boolean canListenNow();
}
