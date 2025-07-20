package config;

import data.Task;

import javax.swing.*;

class Labels extends JPanel {
    Labels(Task task) {
        add(new JLabel(getClass().toGenericString()));
    }
}
