package util;

import javax.swing.*;
import java.awt.*;

public class LabeledComponent extends JPanel {
    public LabeledComponent(String label, JComponent component) {
        this(label, component, null, GridBagConstraints.CENTER);
    }

    public LabeledComponent(String label, JComponent component, String unit) {
        this(label, component, unit, GridBagConstraints.CENTER);
    }

    public LabeledComponent(String label, JComponent component, String unit, int anchor) {
        super(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.anchor = anchor;
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
