package dialogs;

import taskglacier.MainFrame;
import util.DialogEscape;

import javax.swing.*;
import java.awt.*;
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

        setTitle("About");
        setModalityType(ModalityType.APPLICATION_MODAL);
        setDefaultCloseOperation(WindowConstants.DISPOSE_ON_CLOSE);
        setResizable(false);

        DialogEscape.addEscapeHandler(this);

        JPanel content = new JPanel();
        content.setLayout(new GridBagLayout());
        content.setBorder(BorderFactory.createEmptyBorder(12, 16, 16, 16));

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.weightx = 1;
        gbc.weighty = 0;
        gbc.gridwidth = 2;
        gbc.insets = new Insets(5, 5, 5, 5);

        JLabel title = new JLabel("Task Glacier");
        title.setFont(title.getFont().deriveFont(Font.BOLD | Font.ITALIC, 20f));
//        title.setAlignmentX(Component.CENTER_ALIGNMENT);
        content.add(title, gbc);
        gbc.gridy++;

        gbc.fill = GridBagConstraints.HORIZONTAL;

        content.add(new JSeparator(), gbc);
        gbc.gridy++;

        gbc.fill = GridBagConstraints.NONE;
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.gridwidth = 1;

        content.add(new JLabel("UI Version:"), gbc);
        gbc.gridx++;

        content.add(new JLabel(uiVersion + " "), gbc);
        gbc.gridx = 0;
        gbc.gridy++;

        content.add(new JLabel("Server Version:"), gbc);
        gbc.gridx++;

        content.add(new JLabel(serverVersion != null ? serverVersion + " " : "unknown "), gbc);
        gbc.gridx = 0;
        gbc.gridy++;

        add(content);

        pack();

        setLocationRelativeTo(mainFrame);
    }
}
