import org.json.JSONObject;

import javax.swing.*;
import java.awt.*;
import java.io.DataOutputStream;
import java.io.IOException;

public class AddTask extends JDialog {
    public AddTask(DataOutputStream output) {
        // name, time tracking and project info
        // some of this info can be automatically filled (and maybe locked) in the future based on the list we're adding to
        // right now lists don't exist
        JTextField name = new JTextField();

        JTextField timeA = new JTextField();
        JTextField timeB = new JTextField();

        JTextField project = new JTextField();

        JButton add = new JButton("Add");
        add.addActionListener(e -> {
            JSONObject addTask = new JSONObject();
            addTask.put("command", 2);
            addTask.put("name", name.getText());

            try {
                MainFrame.sendJSON(output, addTask);
            } catch (IOException ex) {
                throw new RuntimeException(ex);
            }

            AddTask.this.dispose();
        });

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.insets = new Insets(5, 5, 5 ,5);
        setLayout(new GridBagLayout());

        add(createFlow("Name:", name), gbc);
        gbc.gridy++;

        add(createFlow("Time A:", timeA), gbc);
        gbc.gridy++;

        add(createFlow("Time B:", timeB), gbc);
        gbc.gridy++;

        add(createFlow("Project:", project), gbc);
        gbc.gridy++;

        add(add, gbc);
        gbc.gridy++;

        pack();
    }

    JPanel createFlow(String name, JComponent comp) {
        JPanel panel = new JPanel(new FlowLayout());
        panel.add(new JLabel(name));
        panel.add(comp);
        return panel;
    }
}
