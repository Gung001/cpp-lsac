import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.0
import QtQuick.Controls.Styles 1.1


Rectangle {
	id: buttonActionContainer
	property string disableStateImg
	property string enabledStateImg
	signal clicked

	function enabled(state)
	{
		buttonAction.enabled = state;
		if (state)
			debugImg.iconSource = enabledStateImg;
		else
			debugImg.iconSource = disableStateImg;
	}

	color: "transparent"
	Button
	{
		anchors.fill: parent
		id: debugImg
		iconSource: enabledStateImg
		action: buttonAction
		width: buttonActionContainer.width - 3
		height: buttonActionContainer.height
	}

	Action {
		id: buttonAction
		onTriggered: {
			buttonActionContainer.clicked();
		}
	}
}
