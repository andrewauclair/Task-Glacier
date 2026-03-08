package dialogs;

import taskglacier.MainFrame;
import util.DialogEscape;

import javax.swing.*;
import java.io.IOException;
import java.util.Properties;

public class About extends JDialog {
    public static String serverVersion;
    private static final String uiVersion = loadVersion();

    private static String loadVersion() {
        try (var is = About.class.getResourceAsStream("/version.properties")) {
            if (is == null) return "unknown";
            var props = new Properties();
            props.load(is);
            return props.getProperty("version", "unknown");
        } catch (IOException e) {
            return "unknown";
        }
    }

    public About(MainFrame mainFrame) {
        super(mainFrame);

        setSize(200, 200);
        setTitle("About");
        setModalityType(ModalityType.APPLICATION_MODAL);
        setDefaultCloseOperation(WindowConstants.DISPOSE_ON_CLOSE);

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
