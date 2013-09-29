import QtQuick 2.0
import Ubuntu.Components 0.1
import Ubuntu.Components.Popups 0.1


MainView {
    // objectName for functional testing purposes (autopilot-qt5)
    id: mview
    objectName: "terminal"
    applicationName: "ubuntu-terminal-app"
    automaticOrientation: false

    width: units.gu(50)
    height: units.gu(75)

    Component {
        id: actionSelectionPopover

        ActionSelectionPopover {
            objectName: "panelpopover"
            actions: ActionList {
                Action {
                    objectName: "controlkeysaction"
                    text: i18n.tr("Control keys")
                    onTriggered: pgTerm.showExtraPanel(1)
                }
                Action {
                    objectName: "functionkeysaction"
                    text: i18n.tr("Function keys")
                    onTriggered: pgTerm.showExtraPanel(2)
                }
                Action {
                    objectName: "textkeysaction"
                    text: i18n.tr("Text ctrl keys")
                    onTriggered: pgTerm.showExtraPanel(3)
                }
                Action {
                    objectName: "hidepanelaction"
                    text: i18n.tr("Hide extra panel")
                    onTriggered: pgTerm.showExtraPanel(0)
                }
            }
        }
    }

    Tabs {
        id: tabs
        objectName: "rootTabs"
        anchors.fill: parent

        Tab {
            objectName: "TerminalTab"
            title: i18n.tr("Terminal")
            page: Page {

                onWidthChanged: {
                    header.visible = width < height;
                }

                onHeightChanged: {
                    header.visible = width < height;
                }

                Terminal {
                    id: pgTerm
                    objectName: "pgTerm"
                }

                tools: ToolbarItems {
                    ToolbarButton {
                           id: toolbarAction
                           objectName: "PanelsButton"
                           action: Action {
                                text: "Panels"
                                iconSource: Qt.resolvedUrl("avatar.png")
                                onTriggered: PopupUtils.open(actionSelectionPopover, toolbarAction)
                           }
                    }
                }

            }

        }

        Tab {
            objectName: "SettingsTab"
            title: i18n.tr("Settings")
            page: Page {
                Configs { id: pgConf }
            }
        }

    }
}
