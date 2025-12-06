package util;

import dialogs.TimeEntryConfiguration;

import javax.swing.*;
import java.awt.event.ActionEvent;
import java.awt.event.KeyEvent;

public class DialogEscape {
    public static void addEscapeHandler(JDialog dialog) {
        InputMap inputMap = ((JComponent) dialog.getContentPane()).getInputMap(JComponent.WHEN_IN_FOCUSED_WINDOW);
        ActionMap actionMap = ((JComponent) dialog.getContentPane()).getActionMap();

        KeyStroke ESCAPE_KEY = KeyStroke.getKeyStroke(KeyEvent.VK_ESCAPE, 0);

        inputMap.put(ESCAPE_KEY, "escape");
        actionMap.put("escape", new AbstractAction() {
            @Override
            public void actionPerformed(ActionEvent e) {
                dialog.dispose();
            }
        });
    }
}
