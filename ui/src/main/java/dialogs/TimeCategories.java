package dialogs;

import taskglacier.MainFrame;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.HashMap;
import java.util.Map;

public class TimeCategories extends JDialog {
    JComboBox<String> timeCategorySelection = new JComboBox<>();
    Map<String, DefaultTableModel> timeCodeModels = new HashMap<>();
    DefaultTableModel currentTimeCodeModel = null;

    public TimeCategories(MainFrame mainFrame) {
        setModal(true);

        // Time Category - Label - In Use - Count - Archived
        // Project Server - PS - Yes - 10 - No
        // SAP - SAP - Yes - 12 - No
        // Something - SO - No - 5 - Yes

        // Time Code - In Use - Task Count - Archived
        // G101 - Yes - 100 - No
        // G102 - No - 10 - Yes

        JTabbedPane tabs = new JTabbedPane();

        tabs.add("Categories", setupTimeCategoriesTable());
        tabs.add("Codes", setupCodes());


        JButton save = new JButton("Save");

        setLayout(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.gridwidth = 1;
        gbc.gridheight = 1;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.weightx = 1;
        gbc.weighty = 1;
        add(tabs, gbc);
        gbc.weightx = 0;
        gbc.weighty = 0;
        gbc.fill = GridBagConstraints.NONE;
        gbc.gridy++;
        gbc.anchor = GridBagConstraints.CENTER;
        add(save, gbc);

        pack();

        // center on the main frame
        setLocationRelativeTo(mainFrame);
    }

    private JScrollPane setupTimeCategoriesTable() {
        DefaultTableModel categoriesModel = new DefaultTableModel(new Object[] { "Time Category", "Label", "In Use", "Count", "Archived" }, 0);
        JTable categories = new JTable(categoriesModel);



        JPopupMenu contextMenu = new JPopupMenu();
        JMenuItem add = new JMenuItem("Add...");

        contextMenu.add(add);

        class CreateTimeCategory extends JDialog {
            CreateTimeCategory() {
                setModal(true);

                JTextField category = new JTextField();
                JTextField label = new JTextField();

                JButton add = new JButton();
                add.addActionListener(e -> {
                    categoriesModel.addRow(new Object[] { category.getText(), label.getText(), "No", "0", "No" });
                    timeCodeModels.put(category.getText(), new DefaultTableModel(new Object[] { "Time Code", "In Use", "Task Count", "Archived" }, 0));
                    timeCategorySelection.addItem(category.getText());
                    CreateTimeCategory.this.dispose();
                });

                setLayout(new FlowLayout());

                add(category);
                add(label);
                add(add);

                pack();

                setLocationRelativeTo(TimeCategories.this);
            }
        }
        add.addActionListener(e -> new CreateTimeCategory().setVisible(true));

        categories.addMouseListener(new MouseAdapter() {
            @Override
            public void mouseClicked(MouseEvent e) {
                if (SwingUtilities.isRightMouseButton(e)) {
                    contextMenu.show(categories, e.getX(), e.getY());
                }
            }
        });

        categories.setFillsViewportHeight(true);

        JScrollPane scroll = new JScrollPane(categories);

//        scroll.addMouseListener(new MouseAdapter() {
//            @Override
//            public void mouseClicked(MouseEvent e) {
//                if (SwingUtilities.isRightMouseButton(e)) {
//                    contextMenu.show(scroll, e.getX(), e.getY());
//                }
//            }
//        });

        return scroll;
    }

    private JPanel setupCodes() {
        JPanel panel = new JPanel(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.gridwidth = 1;
        gbc.gridheight = 1;
        gbc.fill = GridBagConstraints.NONE;

        panel.add(timeCategorySelection, gbc);

        JTable codes = new JTable(null);

        timeCategorySelection.addItemListener(e -> {
            currentTimeCodeModel = timeCodeModels.get((String) e.getItem());
            codes.setModel(currentTimeCodeModel);
        });

        JPopupMenu contextMenu = new JPopupMenu();
        JMenuItem add = new JMenuItem("Add...");

        contextMenu.add(add);

        class CreateTimeCode extends JDialog {
            CreateTimeCode() {
                setModal(true);

                JTextField code = new JTextField();

                JButton add = new JButton();
                add.addActionListener(e -> {
                    currentTimeCodeModel.addRow(new Object[] { code.getText(), "No", "0", "No" });
                    CreateTimeCode.this.dispose();
                });

                setLayout(new FlowLayout());

                add(code);
                add(add);

                pack();

                setLocationRelativeTo(TimeCategories.this);
            }
        }
        add.addActionListener(e -> new CreateTimeCode().setVisible(true));

        codes.addMouseListener(new MouseAdapter() {
            @Override
            public void mouseClicked(MouseEvent e) {
                if (SwingUtilities.isRightMouseButton(e)) {
                    contextMenu.show(codes, e.getX(), e.getY());
                }
            }
        });

        codes.setFillsViewportHeight(true);

        JScrollPane scroll = new JScrollPane(codes);

//        scroll.addMouseListener(new MouseAdapter() {
//            @Override
//            public void mouseClicked(MouseEvent e) {
//                if (SwingUtilities.isRightMouseButton(e)) {
//                    contextMenu.show(scroll, e.getX(), e.getY());
//                }
//            }
//        });

        gbc.fill = GridBagConstraints.BOTH;
        gbc.weightx = 1;
        gbc.weighty = 1;
        gbc.gridy++;
        panel.add(scroll, gbc);

        return panel;
    }
}
