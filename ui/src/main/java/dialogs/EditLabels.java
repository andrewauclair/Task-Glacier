package dialogs;

import data.Task;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableModel;
import java.awt.*;
import java.util.ArrayList;
import java.util.List;

import static taskglacier.MainFrame.mainFrame;

public class EditLabels extends JDialog {
    public EditLabels(Task task) {
        setLayout(new GridBagLayout());
        setModal(true);

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;

        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        JButton add = new JButton("Add");
        JButton remove = new JButton("Remove");

        DefaultTableModel model = new DefaultTableModel(new Object[] { "Label" }, 0);
        JTable table = new JTable(model);

        gbc.gridheight = 2;
        add(new JScrollPane(table), gbc);
        gbc.weightx = 0;
        gbc.weighty = 0;
        gbc.fill = GridBagConstraints.NONE;
        gbc.gridheight = 1;
        gbc.gridx++;
        add(add, gbc);
        gbc.gridy++;
        add(remove, gbc);
        gbc.gridx = 0;
        gbc.gridy = 2;

        JPanel buttons = new JPanel();
        JButton save = new JButton("Save");
        JButton cancel = new JButton("Cancel");
        buttons.add(save);
        buttons.add(cancel);
        add(buttons, gbc);

        setSize(300, 300);

        // center on the main frame
        setLocationRelativeTo(mainFrame);

        add.addActionListener(e -> {
            String newLabel = JOptionPane.showInputDialog("New Label");
            model.addRow(new Object[] { newLabel });
        });

        remove.addActionListener(e -> {

        });

        save.addActionListener(e -> {
            task.labels = new ArrayList<>();

            for (int i = 0; i < model.getRowCount(); i++) {
                task.labels.add((String) model.getValueAt(i, 0));
            }

            dispose();
        });

        cancel.addActionListener(e -> dispose());
    }
}
