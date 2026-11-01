/*
 * Registers of the STMicroelectronics M24M01E-U / M24M02E-U I2C EEPROMs:
 * device type identifier (DTI), unique identifier (UID),
 * configurable device address (CDA) and software write protection (SWP).
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef DIALOGI2CREG_H
#define DIALOGI2CREG_H

#include <QDialog>

namespace Ui {
class DialogI2CReg;
}

class DialogI2CReg : public QDialog
{
    Q_OBJECT

public:
    explicit DialogI2CReg(QWidget *parent = nullptr);
    ~DialogI2CReg();
    void setDeviceType(uint8_t pType);
    void setAlgorithm(uint8_t currentAlg);
    void setChipAddr(uint8_t addr);
    void setBusSpeed(uint16_t speed);
    void closeEvent(QCloseEvent* event);

    static bool hasRegisters(const QString &chipName);

private slots:
    void on_pushButton_read_clicked();
    void on_pushButton_writeCda_clicked();
    void on_pushButton_writeSwp_clicked();

signals:
    void closeRequestHasArrived();
    void chipAddrChanged(uint8_t addr);

private:
    Ui::DialogI2CReg *ui;
    uint8_t programmerType;
    QString programmerName;
    uint8_t algorithm;
    uint8_t chipAddr;      // C2 C1 bits of the device select code
    uint8_t cdaMask;       // C2 C1 bits, configurable by this chip
    uint16_t busSpeed;
    bool openProgrammer();
    int regRead(uint16_t address, uint8_t *buf, uint8_t devAddr);
    int regWrite(uint16_t address, uint8_t value, uint8_t devAddr);
    void showCda(uint8_t cda);
    void showSwp(uint8_t swp);
    void setWriteEnabled(bool cdaEnabled, bool swpEnabled);
};

#endif // DIALOGI2CREG_H
