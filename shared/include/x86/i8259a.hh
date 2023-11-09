#pragma once

#include <defs.hh>

u8 Hal8259ARead(bool slave);
u8 Hal8259AReadCommand(bool slave);
void Hal8259AWrite(u8 data, bool slave);
void Hal8259AWriteCommand(u8 data, bool slave);
void Hal8259AMaskAssign(u16 mask);
void Hal8259AMaskSingleSet(int irq);
void Hal8259AMaskSingleClear(int irq);
void Hal8259ASendEoi(bool to_slav);
void Hal8259AInitialize(int remap_master, int remap_slave);
bool Hal8259AIsSpuriousIrq(int irq);
void Hal8259ADisable();