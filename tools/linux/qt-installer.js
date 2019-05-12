function Controller() {
    installer.autoRejectMessageBoxes();
    installer.installationFinished.connect(function() {
        gui.clickButton(buttons.NextButton);
    })
}

Controller.prototype.LicenseCheckPageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

// WELCOME -> NEXT
Controller.prototype.WelcomePageCallback = function() {
    // Hope the license check is faster than 3 seconds.
    gui.clickButton(buttons.NextButton, 3000);
}

// LOGIN -> SKIP
Controller.prototype.CredentialsPageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

// SETUP -> NEXT
Controller.prototype.IntroductionPageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

// INSTALLATION FOLDER -> NEXT
Controller.prototype.TargetDirectoryPageCallback = function()
{
    var targetDir = gui.findChild(gui.currentPageWidget(), "TargetDirectoryLineEdit");
    targetDir.text = "/opt/Qt";
    gui.clickButton(buttons.NextButton);
}


// SELECT COMPONENTS -> NEXT
Controller.prototype.ComponentSelectionPageCallback = function() {
    var widget = gui.currentPageWidget();

    widget.deselectAll();

    widget.selectComponent("qt.qt5.5100.gcc_64");

    gui.clickButton(buttons.NextButton);
}

// LICENSE AGREE -> RADIO IAGREE -> NEXT
Controller.prototype.LicenseAgreementPageCallback = function() {
    gui.currentPageWidget().AcceptLicenseRadioButton.setChecked(true);
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ReadyForInstallationPageCallback = function() {
    gui.clickButton(buttons.CommitButton);
}

Controller.prototype.FinishedPageCallback = function() {
    var checkBoxForm = gui.currentPageWidget().LaunchQtCreatorCheckBoxForm;
    if (checkBoxForm && checkBoxForm.launchQtCreatorCheckBox) {
        checkBoxForm.launchQtCreatorCheckBox.checked = false;
    }
    gui.clickButton(buttons.FinishButton);
}
