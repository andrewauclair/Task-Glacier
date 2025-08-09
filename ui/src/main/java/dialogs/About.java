package dialogs;

import taskglacier.MainFrame;

import javax.swing.*;

public class About extends JDialog {
    public static String serverVersion;
    private static final String uiVersion = "0.6.0";

    public About(MainFrame mainFrame) {
        setSize(200, 200);
        setLayout(new BoxLayout(getContentPane(), BoxLayout.PAGE_AXIS));

        add(new JLabel("Task Glacier"));
        add(new JLabel("Server Version: " + serverVersion));
        add(new JLabel("UI Version: " + uiVersion));

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }
}
