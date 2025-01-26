package panels;

import data.Standards;
import data.Task;

import javax.swing.*;
import java.awt.*;

public class TaskInfoSubPanel extends JPanel {
    private Task task = null;

    private JLabel taskID = new JLabel();

    public TaskInfoSubPanel() {
        super(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(Standards.TOP_INSET, Standards.LEFT_INSET, Standards.BOTTOM_INSET, Standards.RIGHT_INSET);
        gbc.gridx = 0;
        gbc.gridy = 0;

        JPanel id = new JPanel();
        id.setLayout(new FlowLayout());

        id.add(new JLabel("ID: "));
        id.add(taskID);

        add(id, gbc);
    }

    public void displayForTask(Task task) {
        taskID.setText(String.valueOf(task.id));
    }
}
