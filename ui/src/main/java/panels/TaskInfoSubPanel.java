package panels;

import data.Standards;
import data.Task;

import javax.swing.*;
import java.awt.*;
import java.time.Instant;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;

public class TaskInfoSubPanel extends JPanel {
    private Task task = null;

    private JLabel taskID = new JLabel();
    private JLabel createTime = new JLabel();
    private JLabel finishTime = new JLabel();

    private JPanel times = new JPanel();

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
        gbc.gridy++;

        JPanel create = new JPanel();

        create.add(new JLabel("Create Time: "));
        create.add(createTime);

        add(create, gbc);
    }

    private void clear() {
        taskID.setText("");
    }

    public void displayForTask(Task task) {
        if (task == null) {
            clear();
            return;
        }
        taskID.setText(String.valueOf(task.id));

        DateTimeFormatter dateTimeFormatter = DateTimeFormatter.ofPattern("MM/dd/yyyy hh:mm:ss a");

        createTime.setText(task.createTime.atZone(ZoneId.systemDefault()).format(dateTimeFormatter));


    }
}
