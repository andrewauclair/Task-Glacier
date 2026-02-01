package dialogs;

import com.formdev.flatlaf.FlatClientProperties;
import com.formdev.flatlaf.extras.FlatSVGIcon;
import data.Task;
import net.byteseek.swing.treetable.TreeTableModel;
import panels.Search;
import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.tree.DefaultMutableTreeNode;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;

public class TaskPicker extends JDialog {
    public Task task = null;

    public TaskPicker(MainFrame mainFrame, Task selectedTask) {
        super(mainFrame);

        setModalityType(ModalityType.APPLICATION_MODAL);

        task = selectedTask;

        JButton select = new JButton("Select");
        select.addActionListener(e -> TaskPicker.this.dispose());

        Search search = new Search(mainFrame, true);
        search.newTable.addListSelectionListener(e -> {
            select.setEnabled(search.newTable.getSelectedRowCount() != 0);

            int selectedRow = search.newTable.getSelectedRow();

            if (selectedRow == -1) {
                return;
            }

            Task task = (Task) ((DefaultMutableTreeNode) ((TreeTableModel) search.newTable.getModel()).getNodeAtTableRow(selectedRow)).getUserObject();

            this.task = task;
        });

        if (task != null) {
            SwingUtilities.invokeLater(() -> search.newTable.setSelectedTask(task));
        }

        search.newTable.addMouseListener(new MouseAdapter() {
            @Override
            public void mouseClicked(MouseEvent e) {
                if (e.getClickCount() >= 2 && search.newTable.getSelectedRow() != -1) {
                    select.doClick();
                }
            }
        });
        JTextField searchText = new JTextField(30);

        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.insets = new Insets(5, 5, 5, 5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.fill = GridBagConstraints.BOTH;

        add(search, gbc);

        gbc.weighty = 0;
        gbc.fill = GridBagConstraints.HORIZONTAL;

        gbc.gridy++;
        gbc.weightx = 0;

        add(searchText, gbc);
        gbc.gridy++;

        gbc.fill = GridBagConstraints.NONE;

        add(select, gbc);
        gbc.gridy++;

        FlatSVGIcon searchIcon = new FlatSVGIcon(getClass().getResource("/search-svgrepo-com.svg")).derive(24, 24);

        searchText.putClientProperty(FlatClientProperties.TEXT_FIELD_LEADING_ICON, searchIcon);

        searchText.getDocument().addDocumentListener(new DocumentListener() {
            @Override
            public void insertUpdate(DocumentEvent e) {
                search.setSearchText(searchText.getText());
            }

            @Override
            public void removeUpdate(DocumentEvent e) {
                search.setSearchText(searchText.getText());
            }

            @Override
            public void changedUpdate(DocumentEvent e) {
                search.setSearchText(searchText.getText());
            }
        });

        SwingUtilities.invokeLater(searchText::requestFocusInWindow);

        setSize(550, 400);

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }
}
