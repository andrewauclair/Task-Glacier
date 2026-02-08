package dialogs;

import taskglacier.MainFrame;
import util.DialogEscape;

import javax.swing.*;

public class About extends JDialog {
    public static String serverVersion;
    private static final String uiVersion = "0.14.0";

    public About(MainFrame mainFrame) {
        super(mainFrame);

        setSize(200, 200);
        setTitle("About");
        setModalityType(ModalityType.APPLICATION_MODAL);

        DialogEscape.addEscapeHandler(this);

        JPanel info = new JPanel();

        info.setLayout(new BoxLayout(info, BoxLayout.PAGE_AXIS));
        info.setBorder(BorderFactory.createEmptyBorder(5, 5, 5, 5));

        info.add(new JLabel("Task Glacier"));


        info.add(new JLabel("Server Version: " + serverVersion));

        info.add(new JLabel("UI Version: " + uiVersion));

        add(info);

        // center on the main frame
        setLocationRelativeTo(mainFrame);

        pack();
    }
}
