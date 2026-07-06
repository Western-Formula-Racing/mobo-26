#pragma once

// Hybrid SOC estimator (telemetry only - never feeds back into the state machine):
//  - primary estimate comes from temperature-compensated coulomb counting
//  - continuously nudged toward an OCV estimate backed out of terminal
//    voltage via a first-order (R0/R1/C1) equivalent-circuit model, correcting
//    coulomb-counting drift even under load
//  - hard-anchored to the raw OCV table when the state machine reports IDLE
//    (AIRs open, guaranteed zero current) for long enough to fully relax
//  - persisted to NVS so the estimate survives reboots instead of resetting
//
// Cell model data (OCV/R0/R1/capacity vs. SOC & temperature) is sourced from
// vtc6_correction_tables.csv, embedded as tables in SOC.c.

void initSOC();       // call once at boot (after NVS-owning inits), restores persisted SOC
void socPeriodic();   // call every periodic tick; integrates current and checks for OCV correction
float getSOC();       // current SOC estimate, 0.0 - 100.0 %
void setSOC(float newSOC); // manual override (e.g. external calibration), persists immediately
