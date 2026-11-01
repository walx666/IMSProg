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
#include "dialogi2creg.h"
#include "ui_dialogi2creg.h"
#include <QMessageBox>
#include <QRegularExpression>
#include <unistd.h>
#include "ft232all.h"
extern "C" {
#include "spi_controller.h"
#include "ch34x_i2c.h"
}

// Register addresses (A15..A13) in the 1011 device type identifier space
#define REG_ADDR_IDPAGE 0x0000
#define REG_ADDR_SWP    0xa000
#define REG_ADDR_CDA    0xc000
#define REG_ADDR_DTI    0xe000
#define REG_READ_LEN    16      // UID size, also used for single registers
#define DTI_VALUE       0xb1    // factory value of the DTI register

#define CDA_DAL         0x01
#define SWP_WPL         0x01
#define SWP_BP          0x06
#define SWP_WPA         0x08

static QString hex2(uint8_t value)
{
    return QString::number(value, 16).rightJustified(2, '0').toUpper();
}

DialogI2CReg::DialogI2CReg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogI2CReg)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    programmerType = 0;
    algorithm = 0;
    chipAddr = 0;
    cdaMask = 0;
    busSpeed = 100;
    ui->comboBox_bp->addItem(tr("Upper quarter of memory"), 0);
    ui->comboBox_bp->addItem(tr("Upper half of memory"), 1);
    ui->comboBox_bp->addItem(tr("Upper three quarters of memory"), 2);
    ui->comboBox_bp->addItem(tr("Whole memory"), 3);
    QFont monoFont("Monospace");
    monoFont.setStyleHint(QFont::TypeWriter);
    ui->lineEdit_uid->setFont(monoFont);
    setWriteEnabled(false, false);
}

DialogI2CReg::~DialogI2CReg()
{
    delete ui;
}

bool DialogI2CReg::hasRegisters(const QString &chipName)
{
    return chipName.contains(QRegularExpression("^M24M0[12]E"));
}

void DialogI2CReg::setDeviceType(uint8_t pType)
{
    programmerType = pType;
    if (programmerType < 2)  programmerName = "CH341A";
    if ((programmerType == 2) || (programmerType == 3)) programmerName = "CH347T";
    if (programmerType == 4) programmerName = "FT232H";
}

void DialogI2CReg::setAlgorithm(uint8_t currentAlg)
{
    algorithm = currentAlg;
    // The C bits are the device select bits not used for memory addressing:
    // M24M01E-U - C2 C1, M24M02E-U - C2
    cdaMask = i2cChipEnableBits(algorithm, 0x0e);
    ui->checkBox_c2->setVisible((cdaMask & 0x08) != 0);
    ui->checkBox_c1->setVisible((cdaMask & 0x04) != 0);
}

void DialogI2CReg::setChipAddr(uint8_t addr)
{
    chipAddr = addr & cdaMask;
}

void DialogI2CReg::setBusSpeed(uint16_t speed)
{
    busSpeed = speed;
}

void DialogI2CReg::closeEvent(QCloseEvent* event)
{
    emit closeRequestHasArrived();
    QWidget::closeEvent(event);
}

bool DialogI2CReg::openProgrammer()
{
    if (ProgDeviceInit(programmerType, 1, busSpeed) != 0)
    {
        QMessageBox::about(this, tr("Error"), tr("Programmer ") + programmerName + tr(" is not connected!"));
        return false;
    }
    return true;
}

int DialogI2CReg::regRead(uint16_t address, uint8_t *buf, uint8_t devAddr)
{
    uint8_t chipSelect = I2C_DEVTYPE_REG | devAddr;
    if (programmerType < 4) return ch34xi2cBlockRead(buf, address, REG_READ_LEN, algorithm, programmerType, chipSelect);
    return ft232I2cBlockRead(buf, address, REG_READ_LEN, algorithm, chipSelect);
}

int DialogI2CReg::regWrite(uint16_t address, uint8_t value, uint8_t devAddr)
{
    int res;
    uint8_t chipSelect = I2C_DEVTYPE_REG | devAddr;
    // Only one data byte is allowed, more bytes abort the write cycle
    if (programmerType < 4) res = ch34xi2cBlockWrite(&value, address, 1, 1, algorithm, programmerType, chipSelect);
    else res = ft232I2cBlockWrite(&value, address, 1, 1, algorithm, chipSelect);
    usleep(10000); // tW = 4 ms max
    return res;
}

void DialogI2CReg::showCda(uint8_t cda)
{
    QString cBits;
    if (cdaMask & 0x08) cBits += QString(" C2=%1").arg((cda >> 3) & 1);
    if (cdaMask & 0x04) cBits += QString(" C1=%1").arg((cda >> 2) & 1);
    ui->lineEdit_cda->setText(QString("%1h:%2 DAL=%3").arg(hex2(cda), cBits).arg(cda & CDA_DAL));
    ui->checkBox_c2->setChecked((cda & 0x08) != 0);
    ui->checkBox_c1->setChecked((cda & 0x04) != 0);
    ui->checkBox_dal->setChecked((cda & CDA_DAL) != 0);
}

void DialogI2CReg::showSwp(uint8_t swp)
{
    ui->lineEdit_swp->setText(QString("%1h: WPA=%2 BP=%3 WPL=%4").arg(hex2(swp))
                              .arg((swp & SWP_WPA) >> 3).arg((swp & SWP_BP) >> 1).arg(swp & SWP_WPL));
    ui->checkBox_wpa->setChecked((swp & SWP_WPA) != 0);
    ui->comboBox_bp->setCurrentIndex((swp & SWP_BP) >> 1);
    ui->checkBox_wpl->setChecked((swp & SWP_WPL) != 0);
}

void DialogI2CReg::setWriteEnabled(bool cdaEnabled, bool swpEnabled)
{
    ui->checkBox_c2->setEnabled(cdaEnabled);
    ui->checkBox_c1->setEnabled(cdaEnabled);
    ui->checkBox_dal->setEnabled(cdaEnabled);
    ui->pushButton_writeCda->setEnabled(cdaEnabled);
    ui->checkBox_wpa->setEnabled(swpEnabled);
    ui->comboBox_bp->setEnabled(swpEnabled);
    ui->checkBox_wpl->setEnabled(swpEnabled);
    ui->pushButton_writeSwp->setEnabled(swpEnabled);
}

void DialogI2CReg::on_pushButton_read_clicked()
{
    uint8_t dti[REG_READ_LEN], cda[REG_READ_LEN], swp[REG_READ_LEN], uid[REG_READ_LEN];
    int res;

    if (!openProgrammer()) return;
    res = regRead(REG_ADDR_DTI, dti, chipAddr);
    if (res == 0) res = regRead(REG_ADDR_CDA, cda, chipAddr);
    if (res == 0) res = regRead(REG_ADDR_SWP, swp, chipAddr);
    if (res == 0) res = regRead(REG_ADDR_IDPAGE, uid, chipAddr);
    ProgDeviceClose(programmerType);

    if ((res != 0) || (dti[0] != DTI_VALUE))
    {
        ui->lineEdit_dti->clear();
        ui->lineEdit_uid->clear();
        ui->lineEdit_cda->clear();
        ui->lineEdit_swp->clear();
        setWriteEnabled(false, false);
        QMessageBox::about(this, tr("Error"),
                           tr("The chip does not answer at device select code %1h.\n"
                              "Check the connection and the I2C address (C bits) in the main window.")
                           .arg(hex2(I2C_DEVTYPE_REG | chipAddr)));
        return;
    }

    ui->lineEdit_dti->setText(hex2(dti[0]) + "h");
    QString uidText;
    for (int i = 0; i < REG_READ_LEN; i++) uidText += hex2(uid[i]);
    ui->lineEdit_uid->setText(uidText);
    showCda(cda[0]);
    showSwp(swp[0]);
    setWriteEnabled((cda[0] & CDA_DAL) == 0, (swp[0] & SWP_WPL) == 0);
}

void DialogI2CReg::on_pushButton_writeCda_clicked()
{
    uint8_t newCda = 0, readBack[REG_READ_LEN];
    if (ui->checkBox_c2->isChecked()) newCda |= 0x08;
    if (ui->checkBox_c1->isChecked()) newCda |= 0x04;
    newCda &= cdaMask;
    if (ui->checkBox_dal->isChecked())
    {
        if (QMessageBox::warning(this, tr("Warning"),
                                 tr("Setting DAL locks the configurable device address permanently.\n"
                                    "This cannot be undone. Continue?"),
                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) return;
        newCda |= CDA_DAL;
    }

    if (!openProgrammer()) return;
    int res = regWrite(REG_ADDR_CDA, newCda, chipAddr);
    // After a successful write the chip answers at the new chip enable address
    if (res == 0) res = regRead(REG_ADDR_CDA, readBack, newCda & cdaMask);
    ProgDeviceClose(programmerType);

    if ((res != 0) || (readBack[0] != newCda))
    {
        QMessageBox::about(this, tr("Error"),
                           tr("Error writing the CDA register!\n"
                              "The register may be locked (DAL = 1) or the WC pin is high."));
        return;
    }
    chipAddr = newCda & cdaMask;
    showCda(newCda);
    setWriteEnabled((newCda & CDA_DAL) == 0, ui->pushButton_writeSwp->isEnabled());
    emit chipAddrChanged(chipAddr);
}

void DialogI2CReg::on_pushButton_writeSwp_clicked()
{
    uint8_t newSwp = 0, readBack[REG_READ_LEN];
    if (ui->checkBox_wpa->isChecked()) newSwp |= SWP_WPA;
    newSwp |= static_cast<uint8_t>(ui->comboBox_bp->currentData().toUInt() << 1);
    if (ui->checkBox_wpl->isChecked())
    {
        if (QMessageBox::warning(this, tr("Warning"),
                                 tr("Setting WPL locks the software write protection register permanently.\n"
                                    "This cannot be undone. Continue?"),
                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) return;
        newSwp |= SWP_WPL;
    }

    if (!openProgrammer()) return;
    int res = regWrite(REG_ADDR_SWP, newSwp, chipAddr);
    if (res == 0) res = regRead(REG_ADDR_SWP, readBack, chipAddr);
    ProgDeviceClose(programmerType);

    if ((res != 0) || (readBack[0] != newSwp))
    {
        QMessageBox::about(this, tr("Error"),
                           tr("Error writing the SWP register!\n"
                              "The register may be locked (WPL = 1) or the WC pin is high."));
        return;
    }
    showSwp(newSwp);
    setWriteEnabled(ui->pushButton_writeCda->isEnabled(), (newSwp & SWP_WPL) == 0);
}
