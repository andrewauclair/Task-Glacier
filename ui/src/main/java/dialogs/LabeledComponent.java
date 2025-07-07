package dialogs;

import javax.swing.*;
import java.awt.*;

class LabeledComponent extends JPanel {
    LabeledComponent(String label, JComponent component) {
        super(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.insets = new Insets(0, 0, 0, 0);
        gbc.gridx = 0;
        gbc.gridy = 0;

        add(new JLabel(label), gbc);
        gbc.gridx++;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.weightx = 1;
        gbc.insets = new Insets(0, 5, 0, 0);
        add(component, gbc);
    }
}
